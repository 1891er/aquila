/******************************************************************************
 * Copyright (C) 2014-2018 Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <x264.h>
#ifdef __cplusplus
}
#endif


#include <libmacro.h>
#include <liblog.h>

#include "codec.h"
#include "common.h"
#include "imgconvert.h"

struct x264_ctx {
    int width;
    int height;
    int encode_format;
    int input_format;
    struct iovec sei;
    x264_param_t *param;
    x264_t *handle;
    x264_picture_t *picture;
    x264_nal_t *nal;
    void *parent;
};

#define AV_INPUT_BUFFER_PADDING_SIZE 32

static int x264_open(struct codec_ctx *cc, struct media_params *media)
{
    int m_frameRate = 25;
    //int m_bitRate = 1000;
    struct x264_ctx *c = CALLOC(1, struct x264_ctx);
    if (!c) {
        loge("malloc x264_ctx failed!\n");
        return -1;
    }
    c->param = CALLOC(1, x264_param_t);
    if (!c->param) {
        loge("malloc param failed!\n");
        goto failed;
    }
    c->picture = CALLOC(1, x264_picture_t);
    if (!c->picture) {
        loge("malloc picture failed!\n");
        goto failed;
    }
    x264_param_default_preset(c->param, "ultrafast" , "zerolatency");

    c->param->i_width = media->video.width;
    c->param->i_height = media->video.height;
    //XXX b_repeat_headers 0: rtmp is ok; 1: playback is ok
    c->param->b_repeat_headers = 0;  // repeat SPS/PPS before i frame
    c->param->b_vfr_input = 0;
    c->param->b_annexb = 1;
    c->param->b_cabac = 1;
    c->param->i_threads = 1;
    c->param->i_fps_num = (int)m_frameRate;
    c->param->i_fps_den = 1;
    c->param->i_keyint_max = 25;
    c->param->i_log_level = X264_LOG_NONE;
    x264_param_apply_profile(c->param, "high422");
    c->handle = x264_encoder_open(c->param);
    loge("c->handle = %p\n", c->handle);
    if (c->handle == 0) {
        loge("x264_encoder_open failed!\n");
        goto failed;
    }

    if (-1 == x264_picture_alloc(c->picture, X264_CSP_I420, c->param->i_width,
            c->param->i_height)) {
        loge("x264_picture_alloc failed!\n");
        goto failed;
    }
    c->picture->img.i_csp = X264_CSP_I420;
    c->picture->img.i_plane = 3;
    c->width = media->video.width;
    c->height = media->video.height;
    c->input_format = media->video.pix_fmt;
    c->encode_format = YUV422P;

    if (1) {//flags & AV_CODEC_FLAG_GLOBAL_HEADER) {
        x264_nal_t *nal;
        uint8_t *p;
        int nnal, s, i;

        s = x264_encoder_headers(c->handle, &nal, &nnal);
        cc->extradata.iov_base = p = (uint8_t *)calloc(1, s + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!p)
            return -1;

        for (i = 0; i < nnal; i++) {
            /* Don't put the SEI in extradata. */
            if (nal[i].i_type == NAL_SEI) {
                logd("%s\n", nal[i].p_payload+25);
                c->sei.iov_len = nal[i].i_payload;
                c->sei.iov_base = calloc(1, c->sei.iov_len);
                if (!c->sei.iov_base)
                    return -1;
                memcpy(c->sei.iov_base, nal[i].p_payload, nal[i].i_payload);
                continue;
            }
            memcpy(p, nal[i].p_payload, nal[i].i_payload);
            p += nal[i].i_payload;
        }
        cc->extradata.iov_len = p - (uint8_t *)cc->extradata.iov_base;
    }
    c->parent = cc;
    cc->priv = c;
    return 0;

failed:
    if (c->handle) {
        x264_encoder_close(c->handle);
    }
    if (c) {
        free(c);
    }
    return -1;
}

static int encode_nals(struct x264_ctx *c, struct iovec *pkt,
                       const x264_nal_t *nals, int nnal)
{
    uint8_t *p;
    int i;
    size_t size = c->sei.iov_len;
    struct codec_ctx *cc = (struct codec_ctx *)c->parent;

    if (!nnal)
        return 0;

    for (i = 0; i < nnal; i++)
        size += nals[i].i_payload;

    p = (uint8_t *)calloc(1, size);
    if (!p) {
        return -1;
    }
    pkt->iov_base = p;
    pkt->iov_len = 0;

    static bool append_extradata = false;
    if (!append_extradata) {
        /* Write the extradata as part of the first frame. */
        if (cc->extradata.iov_len > 0 && nnal > 0) {
            if (cc->extradata.iov_len > size) {
                loge("Error: nal buffer is too small\n");
                return -1;
            }
            memcpy(p, cc->extradata.iov_base, cc->extradata.iov_len);
            p += cc->extradata.iov_len;
            pkt->iov_len += cc->extradata.iov_len;
        }
        append_extradata = true;
    }

    for (i = 0; i < nnal; i++){
        memcpy(p, nals[i].p_payload, nals[i].i_payload);
        p += nals[i].i_payload;
        pkt->iov_len += nals[i].i_payload;
    }
    return pkt->iov_len;
}

static int x264_encode(struct codec_ctx *cc, struct iovec *in, struct iovec *out)
{
    struct x264_ctx *c = (struct x264_ctx *)cc->priv;
    x264_picture_t pic_out;
    int nNal = 0;
    int ret = 0;
    int i = 0, j = 0;
    c->nal = NULL;
    uint8_t *p422;

    uint8_t *y = c->picture->img.plane[0];
    uint8_t *u = c->picture->img.plane[1];
    uint8_t *v = c->picture->img.plane[2];

    int widthStep422 = c->param->i_width * 2;

#if 0
    if (c->input_format == YUV420) {
        struct iovec in_tmp;
        in_tmp.iov_len = in->iov_len;
        in_tmp.iov_base = calloc(1, in->iov_len);
        conv_yuv420pto422(in_tmp.iov_base, in->iov_base, c->width, c->height);
        for (i = 0; i < (int)in->iov_len; i++) {//8 byte copy
            *((char *)in->iov_base + i) = *((char *)in_tmp.iov_base + i);
        }
        free(in_tmp.iov_base);
    }
#endif

    for(i = 0; i < c->param->i_height; i += 2) {
        p422 = (uint8_t *)in->iov_base + i * widthStep422;
        for(j = 0; j < widthStep422; j+=4) {
            *(y++) = p422[j];
            *(u++) = p422[j+1];
            *(y++) = p422[j+2];
        }

        p422 += widthStep422;

        for(j = 0; j < widthStep422; j+=4) {
            *(y++) = p422[j];
            *(v++) = p422[j+3];
            *(y++) = p422[j+2];
        }
    }
    c->picture->i_type = X264_TYPE_I;

    do {
        if (x264_encoder_encode(c->handle, &(c->nal), &nNal, c->picture,
                    &pic_out) < 0) {
            return -1;
        }
        ret = encode_nals(c, out, c->nal, nNal);
        if (ret < 0) {
            return -1;
        }
    } while (0);
    c->picture->i_pts++;
    return ret;
}

static void x264_close(struct codec_ctx *cc)
{
    struct x264_ctx *c = (struct x264_ctx *)cc->priv;
    if (c->handle) {
        //x264_encoder_close(c->handle);//XXX cause segfault

        free(cc->extradata.iov_base);
        cc->extradata.iov_len = 0;
    }
    free(c->sei.iov_base);
    free(c);
}

struct codec aq_x264_encoder = {
    .name   = "x264",
    .open   = x264_open,
    .encode = x264_encode,
    .decode = NULL,
    .close  = x264_close,
};
