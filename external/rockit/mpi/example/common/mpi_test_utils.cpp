/*
 * Copyright 2020 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <sys/time.h>
#include <time.h>
#include <cstring>

#include "mpi_test_utils.h"

#define SIZE_ARRAY_ELEMS(a)          (sizeof(a) / sizeof((a)[0]))

RK_U64 mpi_test_utils_get_now_us() {
    struct timespec time = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (RK_U64)time.tv_sec * 1000000 + (RK_U64)time.tv_nsec / 1000; /* microseconds */
}

URI_SCHEME_TYPE get_uri_scheme_type(const char* uri) {
    URI_SCHEME_TYPE schemeType = RK_URI_SCHEME_LOCAL;
    if ((RK_NULL == uri) || (strlen(uri) < 4)) {
        return schemeType;
    }

    if (!strncmp("http://", uri, 7) || !strncmp("https://", uri, 8)) {
        schemeType = RK_URI_SCHEME_HTTP;
    } else if (!strncmp("rtsp://", uri, 7) || !strncmp("rtmp://", uri, 7)) {
        RK_LOGD("uri is with rtsp or rtmp scheme type");
        schemeType = RK_URI_SCHEME_RTSP;
    } else if (!strncmp("/data/smb/", uri, 10)) {
        RK_LOGD("uri is with /data/smb scheme type");
        schemeType = RK_URI_SCHEME_SAMBA;
    } else if (!strncmp("/data/nfs/", uri, 10)) {
        RK_LOGD("uri is with /data/nfs schemeType (signed as samba)");
        schemeType = RK_URI_SCHEME_SAMBA;
    } else if (strstr(uri, "m3u8")) {
        RK_LOGD("uri is with m3u8 scheme type");
        schemeType = RK_URI_SCHEME_HLS;
    } else if (!strncmp("rtp:", uri, 4)) {
        RK_LOGD("uri is with rtp scheme type");
        schemeType = RK_URI_SCHEME_RTP;
    } else if (!strncmp("udp:", uri, 4)) {
        RK_LOGD("uri is with udp scheme type");
        schemeType = RK_URI_SCHEME_UDP;
    } else if (!strncmp("mms://", uri, 6)) {
        RK_LOGD("uri is with mms scheme type");
        schemeType = RK_URI_SCHEME_MMS;
    } else if (!strncmp("mmsh://", uri, 7)) {
        RK_LOGD("uri is with mmsh scheme type");
        schemeType = RK_URI_SCHEME_MMSH;
    } else if (!strncmp("mmst://", uri, 7)) {
        RK_LOGD("uri is with mmst scheme type");
        schemeType = RK_URI_SCHEME_MMST;
    } else if (strstr(uri, "app_tts-cache")) {
        RK_LOGD("uri is with tts scheme type");
        schemeType = RK_URI_SCHEME_TTS;
    }  else if (strstr(uri, "cache://")) {
        schemeType = RK_URI_SCHEME_IPTV;
    }
    return schemeType;
}

