/*
    mp3_prv.h

    Private API's to be used within the mp3* files
*/

#ifndef MP3_PRV_H
#define MP3_PRV_H

#include "MP3_pub.h"

#include "bsp.h"

/*---------------------------------
mp3_main.c
---------------------------------*/

void mp3_set_playback_status
    (
    MP3_playback_sts_type sts
    );

void mp3_signal_buffer_empty
    ( void );

/*---------------------------------
mp3_strm.c
---------------------------------*/

void mp3_strm_pwrp
    ( void );

BOOLEAN mp3_strm_open
    ( void );

BOOLEAN mp3_strm_close
    ( void );

BOOLEAN mp3_strm_write_data
    (
    const INT8U* ptr_data,
    INT32U       data_size
    );

void mp3_strm_pause
    ( void );

BOOLEAN mp3_strm_resume
    ( void );

INT16U mp3_strm_get_decode_time
    (
    void
    );

/*---------------------------------
mp3_strm_util.c
---------------------------------*/

void mp3_strm_util_pwrp
    ( void );

void mp3_strm_util_start
    (
    HANDLE hMp3
    );

void mp3_strm_util_stop
    (
    HANDLE hMp3
    );

void mp3_strm_util_test
    (
    HANDLE hMp3
    );

void mp3_strm_util_stream_data
    (
    HANDLE hMp3,
    INT8U *pBuf,
    INT32U bufLen
    );

INT16U mp3_strm_util_get_decode_time
    (
    HANDLE hMp3
    );

#endif // MP3_PRV_H
