/**
    @file        mp3_stream.c

    @author      Vimal Mehta

    @description
        This thread is responsible for streaming a buffer
    of data to the MP3 decoder, and sending an new
    buffer request event back to the MP3 main thread

    Copyright (c) 2016 Vimal Mehta
*/

// Includes
#include <stdarg.h>
#include "ucos_ii.h"
#include "bsp.h"
#include "MP3_pub.h"
#include "mp3_prv.h"

/**
    Types
*/

// Events handled by the thead
enum
    {
    STRM_EVNT_BUFFER_FULL  = 0x01,

    STRM_EVNT_CNT
    };


// Workspace type
typedef struct
    {
    HANDLE  hndl_mp3;
    HANDLE  hndl_spi;
    } strm_mp3_wksp_type;


/**
    Static Variables
*/
static OS_STK               strm_mp3_main_stack[APP_CFG_TASK_START_STK_SIZE];
static OS_EVENT *           strm_mp3_smphr;
static OS_FLAG_GRP         *strm_event_flags= 0;
static strm_mp3_wksp_type   strm_mp3_wksp;
static INT32U               strm_mp3_data_size;
static BOOLEAN              paused;
static INT8U                strm_mp3_buff[64];


/**
    Static Procedures
*/
void mp3_strm_main
    (
    void* pdata
    );

static void strm_send_evnt
    (
    OS_FLAGS        evnt_flags
    );

static OS_FLAGS strm_wait_for_evnt
    ( void );

static void reserve_smphr
    ( void );

static void release_smphr
    ( void );

/**
    Power up the MP3 streaming thread.

    This function is used to initialize all the
    global variables and also start the MP3 streaming thread.

    @return None
*/
void mp3_strm_pwrp
    ( void )
{
INT8U err;

mp3_strm_util_pwrp();

strm_mp3_smphr  = OSSemCreate( 1 );

strm_mp3_wksp.hndl_mp3  = -1;
strm_mp3_wksp.hndl_spi  = -1;
paused                  = false;
strm_mp3_data_size      = 0;

// Create the event flags for this thread
strm_event_flags = OSFlagCreate( 0x0, &err );

OSTaskCreate
    (
    mp3_strm_main,
    (void*)0,
    &strm_mp3_main_stack[APP_CFG_TASK_START_STK_SIZE-1],
    APP_TASK_MP3_STREAM_MAIN_PRIO
    );
}

/**
    MP3 stream main

    This function is waiting to receive a process data
    event from the MP3 main thread. On receiving this event,
    it empties the data in the strm_buff to the MP3 decoder
    and requests new data from the MP3 main thread.
*/
void mp3_strm_main
    (
    void* pdata
    )
{
OS_FLAGS rx_flags;

for(;;)
    {
    rx_flags = strm_wait_for_evnt();

    if( rx_flags & STRM_EVNT_BUFFER_FULL )
        {
        reserve_smphr();

        // If we are not paused
        if( !paused )
            {

            // If there is data to be buffered
            if( strm_mp3_data_size > 0 )
                {
                // Write data to the stream
                mp3_strm_util_stream_data( strm_mp3_wksp.hndl_mp3, strm_mp3_buff, strm_mp3_data_size );
                strm_mp3_data_size = 0;
                }
            release_smphr();

            // Signal to the MP3 thread that the
            // data has been written to the stream
            mp3_signal_buffer_empty();
            }
        else
            {
            release_smphr();
            }
        }
    }

} /* mp3_strm_main() */

/**
    Open the MP3 stream

    This function opens the MP3 stream
    and inits the MP3 decoder and sets
    it up for playback.
*/

BOOLEAN mp3_strm_open
    ( void )
{
PjdfErrCode pjdfErr;
INT32U      length;
BOOLEAN     success;
HANDLE      spi_hndl;

success     = false;

reserve_smphr();

// Open handle to the MP3 decoder driver
strm_mp3_wksp.hndl_mp3 = Open( PJDF_DEVICE_ID_MP3_VS1053, 0 );
if( !PJDF_IS_VALID_HANDLE( strm_mp3_wksp.hndl_mp3 ) )
    {
    goto exit_open_mp3_handle;
    }

spi_hndl = Open( MP3_SPI_DEVICE_ID, 0 );

if( !PJDF_IS_VALID_HANDLE( spi_hndl ) )
    {
    goto exit_open_mp3_handle;
    }

length = sizeof( HANDLE );

pjdfErr = Ioctl( strm_mp3_wksp.hndl_mp3 , PJDF_CTRL_MP3_SET_SPI_HANDLE, &spi_hndl, &length );

if( PJDF_IS_ERROR( pjdfErr ) )
    {
    goto exit_open_mp3_handle;
    }

mp3_strm_util_start( strm_mp3_wksp.hndl_mp3 );

success             = true;
paused              = false;
strm_mp3_data_size  = 0;

// Send this event so that the MP3 stream
// thread can start requesting buffer data
// from the MP3 main thread.
strm_send_evnt( STRM_EVNT_BUFFER_FULL );

exit_open_mp3_handle:

release_smphr();

return success;

} /* mp3_strm_open()*/

/**
    Close the MP3 stream

    This funciton closes the MP3 stream
*/

BOOLEAN mp3_strm_close
    ( void )
{
BOOLEAN     success;
PjdfErrCode pjdfErr;

reserve_smphr();

success = false;

if( -1 == strm_mp3_wksp.hndl_mp3 )
    {
    success = true;
    }
else
    {
    mp3_strm_util_stop( strm_mp3_wksp.hndl_mp3 );

    pjdfErr = Close( strm_mp3_wksp.hndl_mp3 );

    if( PJDF_IS_ERROR( pjdfErr ) )
        {
        success = false;
        }
    else
        {
        success                = true;
        strm_mp3_wksp.hndl_mp3 = -1;
        }

    paused              = false;
    strm_mp3_data_size  = 0;
    }

release_smphr();

return success;
} /* mp3_strm_close() */


/**
    Get the MP3 decode time

    This function is used to get the current
    MP3 decoder playback time.
*/

INT16U mp3_strm_get_decode_time
    (
    void
    )
{
INT16U      ret_val;

ret_val = 0;

reserve_smphr();

if( strm_mp3_wksp.hndl_mp3 != -1 )
    {
    ret_val = mp3_strm_util_get_decode_time( strm_mp3_wksp.hndl_mp3 );
    }

release_smphr();

return ret_val;
}

/**
    Write data to the MP3 stream

    This function copies of the incoming
    MP3 data to a buffer and sends an
    event to the MP3 stream thread to send
    that data to the MP3 decoder.
*/

BOOLEAN mp3_strm_write_data
    (
    const INT8U* ptr_data,
    INT32U       data_size
    )
{
BOOLEAN success;

success = false;

reserve_smphr();

if( ( data_size > 0 ) && ( 0 == strm_mp3_data_size ) )
    {
    memcpy( strm_mp3_buff, ptr_data, data_size );
    strm_mp3_data_size = data_size;
    strm_send_evnt( STRM_EVNT_BUFFER_FULL );
    }

release_smphr();

return success;
} /* mp3_strm_write_data() */

/**
    Pause the MP3 streaming thread
*/

void mp3_strm_pause
    ( void )
{

reserve_smphr();
paused = true;
release_smphr();

} /* mp3_strm_pause() */

/**
    Resume the MP3 streaming thread

    @return Returns TRUE if there is
    buffered up data already present
*/

BOOLEAN mp3_strm_resume
    ( void )
{
BOOLEAN pending_data_in_buffer;

pending_data_in_buffer = false;

reserve_smphr();

paused = false;

if( strm_mp3_data_size > 0 )
    {
    pending_data_in_buffer = true;
    strm_send_evnt( STRM_EVNT_BUFFER_FULL );
    }

release_smphr();

return pending_data_in_buffer;

} /* mp3_strm_resume() */


/**
    Send an event
*/

static void strm_send_evnt
    (
    OS_FLAGS        evnt_flags
    )
{

INT8U err;
OSFlagPost( strm_event_flags, evnt_flags, OS_FLAG_SET, &err );

} /* strm_send_evnt() */

/**
    Wait for an event
*/
static OS_FLAGS strm_wait_for_evnt
    ( void )
{

INT8U       err;
OS_FLAGS    flags;

OSFlagPend( strm_event_flags, 0xFF, OS_FLAG_WAIT_SET_ANY, 0, &err );

flags = strm_event_flags->OSFlagFlags;

OSFlagPost( strm_event_flags, 0xFF, OS_FLAG_CLR, &err );

return flags;
} /* strm_wait_for_evnt() */


/**
    Reserve a sempahore
*/

static void reserve_smphr
    ( void )
{
INT8U err;

OSSemPend( strm_mp3_smphr, 0, &err );
}


/**
    Release a sempahore
*/
static void release_smphr
    ( void )
{
OSSemPost( strm_mp3_smphr );
}
