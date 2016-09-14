/*
    MP3_pub.h
    Pubic API's for MP3 task
*/

#ifndef MP3_PUB_H
#define MP3_PUB_H

#include "bsp.h"

#define MP3_PLAYBACK_FILE_NAME_LEN_MAX      ( 32 )

typedef INT8U MP3_playback_sts_type; enum
    {
    MP3_PLAYBACK_STS_OFF            = 0,
    MP3_PLAYBACK_STS_INIT           = 1,
    MP3_PLAYBACK_STS_IN_PROGRESS    = 2,
    MP3_PLAYBACK_STS_PAUSE          = 3,
    MP3_PLAYBACK_STS_RESUME         = 4,
    MP3_PLAYBACK_STS_DONE           = 5,

    MP3_PLAYBACK_STS_CNT
    };

void MP3_pwrp
    ( void );

BOOLEAN MP3_playback_is_plybk_in_prog
    ( void );

BOOLEAN MP3_playback_start
    (
    const char* ptr_file_name
    );

BOOLEAN MP3_playback_stop
    ( void );

BOOLEAN MP3_playback_pause
    ( void );

BOOLEAN MP3_playback_resume
    ( void );

INT16U MP3_playback_get_time_scnds
    ( void );

MP3_playback_sts_type MP3_playback_get_status
    ( void );

#endif
