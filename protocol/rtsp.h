/******************************************************************************
 * Copyright (C) 2014-2016
 * file:    rtsp.h
 * author:  gozfree <gozfree@163.com>
 * created: 2016-05-22 22:36
 * updated: 2016-05-22 22:36
 ******************************************************************************/
#ifndef _RTSP_H_
#define _RTSP_H_


#ifdef __cplusplus
extern "C" {
#endif


#define  ALLOWED_COMMAND \
    "OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, GET_PARAMETER, SET_PARAMETER"

#define RESP_BAD_REQUEST_FMT    ((const char *) \
        "RTSP/1.0 400 Bad Request\r\n"          \
        "%s\r\n"                                \
        "Allow: %s\r\n\r\n")

#define RESP_NOT_SUPPORTED_FMT  ((const char *) \
        "RTSP/1.0 405 Method Not Allowed\r\n"   \
        "CSeq: %s\r\n"                          \
        "%s\r\n"                                \
        "Allow: %s\r\n\r\n")

#define RESP_NOT_FOUND_FMT      ((const char *) \
        "RTSP/1.0 404 Stream Not Found\r\n"     \
        "CSeq: %s\r\n"                          \
        "%s\r\n")

#define RESP_SESSION_NOT_FOUND_FMT ((const char *) \
        "RTSP/1.0 454 Session Not Found\r\n"     \
        "CSeq: %s\r\n"                          \
        "%s\r\n")

#define RESP_UNSUPPORTED_TRANSPORT_FMT ((const char *) \
        "RTSP/1.0 461 Unsupported Transport\r\n"\
        "CSeq: %s\r\n"                          \
        "%s\r\n")

#define RESP_OPTIONS_FMT    ((const char *)     \
        "RTSP/1.0 200 OK\r\n"                   \
        "CSeq: %s\r\n"                          \
        "%s\r\n"                                \
        "Public: %s\r\n\r\n")

#define RESP_DESCRIBE_FMT   ((const char *)     \
        "RTSP/1.0 200 OK\r\n"                   \
        "CSeq: %s\r\n"                          \
        "%s\r\n"                                \
        "Content-Base: %s\r\n"                  \
        "Content-Type: application/sdp\r\n"     \
        "Content-Length: %u\r\n\r\n"            \
        "%s")

#define RESP_SETUP_FMT      ((const char *)     \
        "RTSP/1.0 200 OK\r\n"                   \
        "CSeq: %s\r\n"                          \
        "%s\r\n"                                \
        "Transport: %s;unicast;destination=%s;source=%s;"   \
        "client_port=%hu-%hu;server_port=%hu-%hu\r\n"   \
        "Session: %08X\r\n\r\n")

#define RESP_PLAY_FMT       ((const char *)     \
        "RTSP/1.0 200 OK\r\n"                   \
        "CSeq: %s\r\n"                          \
        "%s\r\n"                                \
        "Range: npt=0.000-\r\n"                 \
        "Session: %08X\r\n"                     \
        "%s\r\n\r\n")

#define SDP_FILTER_FMT      ((const char *)     \
        "a=source-filter: incl IN IP4 * %s\r\n" \
        "a=rtcp-unicast: reflection\r\n")



#ifdef __cplusplus
}
#endif
#endif
