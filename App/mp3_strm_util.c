/*
    @file        mp3_strm.c

    Some utility functions for controlling the MP3 decoder.

    Developed for University of Washington embedded systems programming certificate

    2016/2 Nick Strathy wrote/arranged it
*/

// Includes
#include "mp3_prv.h"
#include "bsp.h"
#include "print.h"
#include "pjdf.h"

/**
    Static Procedures
*/

/**
    Power up the MP3 streaming utility module.

*/
void mp3_strm_util_pwrp
    ( void )
{

} /* mp3_strm_util_pwrp() */

/**
    Utility function to start the initialize the
    MP3 driver and set it up for streaming

*/
void mp3_strm_util_start
    (
    HANDLE hMp3
    )
{

INT32U length;

// Place MP3 driver in command mode (subsequent writes will be sent to the decoder's command interface)
Ioctl(hMp3, PJDF_CTRL_MP3_SELECT_COMMAND, 0, 0);

// Reset the device
length = BspMp3SoftResetLen;
Write(hMp3, (void*)BspMp3SoftReset, &length);

length = BspMp3SetClockFLen;
Write(hMp3, (void*)BspMp3SetClockF, &length);

// Set volume
length = BspMp3SetVol1010Len;
Write(hMp3, (void*)BspMp3SetVol1010, &length);

// To allow streaming data, set the decoder mode to Play Mode
length = BspMp3PlayModeLen;
Write(hMp3, (void*)BspMp3PlayMode, &length);

} /* mp3_strm_util_start() */


/**
    Utility function to stop the MP3 driver
*/

void mp3_strm_util_stop
    (
    HANDLE hMp3
    )
{

Ioctl(hMp3, PJDF_CTRL_MP3_SELECT_COMMAND, 0, 0);

} /* mp3_strm_util_stop()*/

/**
    Utility function to stream data to the MP3 decoder
    MP3 driver and set it up for streaming
*/

void mp3_strm_util_stream_data
    (
    HANDLE hMp3,
    INT8U *pBuf,
    INT32U bufLen
    )
{
INT8U *bufPos = pBuf;
INT32U iBufPos = 0;
INT32U chunkLen;
BOOLEAN done = OS_FALSE;

chunkLen = MP3_DECODER_BUF_SIZE;

// Set MP3 driver to data mode (subsequent writes will be sent to decoder's data interface)
Ioctl(hMp3, PJDF_CTRL_MP3_SELECT_DATA, 0, 0);

while( !done )
    {
    // detect last chunk of pBuf
    if (bufLen - iBufPos < MP3_DECODER_BUF_SIZE)
        {
        chunkLen = bufLen - iBufPos;
        done = OS_TRUE;
        }

    Write(hMp3, bufPos, &chunkLen);
    OSTimeDly(1);

    bufPos += chunkLen;
    iBufPos += chunkLen;
    }

} /* mp3_strm_util_stream_data()*/

/**
    Utility function to get the current decoder
    time

    @return returns the current decoder time

*/

INT16U mp3_strm_util_get_decode_time
    (
    HANDLE hMp3
    )
{
INT8U       buf[10];
INT32U      len;
INT16U      decode_time;

decode_time = 0;

if ( PJDF_IS_VALID_HANDLE( hMp3 ) )
    {
    Ioctl(hMp3, PJDF_CTRL_MP3_SELECT_COMMAND, 0, 0);

    memcpy(buf, BspMp3ReadDecodeTime, BspMp3ReadDecodeTimeLen); // copy command from flash to a ram buffer

    len  = BspMp3ReadDecodeTimeLen;

    if( PJDF_ERR_NONE == Read(hMp3, buf, &len) )
        {
        decode_time = ( ( (INT16U)buf[2] ) << 8 ) & ( (INT16U)0xFF00 );
        decode_time = decode_time | ( (INT16U)( (INT16U)buf[3] & (INT16U)0x00FF ) );
        Ioctl( hMp3, PJDF_CTRL_MP3_SELECT_DATA, 0, 0 );
        }
    else
        {
        while(1);
        }
    }
else
    {
    while(1);
    }

return decode_time;
} /* mp3_strm_util_get_decode_time()*/