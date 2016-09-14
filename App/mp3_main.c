/**
    @file        mp3_main.c

    @author      Vimal Mehta

    @description
        This is the main task that handles various events
    associated with operation of an MP3 player. This task
    is also responsible for setting up the MP3 stream,
    reading a buffer of MP3 data from the MP3 file and
    seding that buffer to the MP3 streaming thread.

    Copyright (c) 2016 Vimal Mehta
*/

// Includes
#include <stdarg.h>
#include "ucos_ii.h"
#include "bsp.h"
#include "SD.h"
#include "MP3_pub.h"
#include "mp3_prv.h"

/**
    Types
*/

// Events handled by the MP3 main thead
enum
    {
    EVNT_PLAY_START         = 0x01,
    EVNT_PLAY_STOP          = 0x02,
    EVNT_PLAY_PAUSE         = 0x04,
    EVNT_PLAY_RESUME        = 0x08,
    EVNT_BUFFER_EMPTY       = 0x10,
    EVNT_RX_MSG             = 0x20,

    EVNT_PLAY_CNT
    };

// Workspace type
typedef struct
    {
    BOOLEAN                 file_hndl_valid;
    File                    file_hndl;
    MP3_playback_sts_type   cur_playback_status;
    } main_mp3_wksp_type;

// Message type
typedef struct
    {
    char fname[MP3_PLAYBACK_FILE_NAME_LEN_MAX];
    } msg_type;

/**
    Static Variables
*/
static OS_STK                   main_mp3_stack[APP_CFG_TASK_START_STK_SIZE];        // MP3 main stack
static char                     cur_mp3_plbk_fname[MP3_PLAYBACK_FILE_NAME_LEN_MAX]; // Name of the playback file name
static OS_FLAG_GRP              *rx_events_mp3 = 0;                                 // Event flags
static OS_EVENT *               intf_smphr_mp3;                                     // Sempahore to protect access to global variables
static INT8U                    strm_buff[64];                                      // Buffer to copy MP3 data from MP3 file
static main_mp3_wksp_type       wksp_mp3;                                           // Workspace
static OS_EVENT *               main_mp3_msg_box;                                   // Message box

/**
    Static Procedures
*/

void mp3_main
    (
    void* pdata
    );

static void send_evnt
    (
    OS_FLAGS        evnt_flags
    );

static OS_FLAGS wait_for_evnt
    ( void );

static MP3_playback_sts_type get_playback_status
    ( void );

static void set_playback_status
    (
    MP3_playback_sts_type sts
    );

static void set_playback_fname
    (
    const char* ptr_file_name
    );

static BOOLEAN add_data_to_buffer
    (
    INT32U* ptr_size
    );

static BOOLEAN start_playback
    ( void );

static void stop_playback
    ( void );

static BOOLEAN is_file_loaded
    (
    void
    );

static BOOLEAN send_msg
    (
    const char* ptr_file_name
    );

/**
    Power up the MP3 main thread.

    This function is used to initialize all the
    global variables and also start the MP3 main thread.

    @return None
*/
void MP3_pwrp
    ( void )
{
INT8U err;

// Ceate the message box
main_mp3_msg_box = OSMboxCreate( NULL );

// Create the semaphore
intf_smphr_mp3  = OSSemCreate( 1 );

// Create the event flags for this thread
rx_events_mp3 = OSFlagCreate( 0x0, &err );

// Initalize the global variables
cur_mp3_plbk_fname[0]           = '\0';
wksp_mp3.file_hndl_valid        = false;
wksp_mp3.cur_playback_status    = MP3_PLAYBACK_STS_OFF;

// Create the MP3 main thread
OSTaskCreate
    (
    mp3_main,
    (void*)0,
    &main_mp3_stack[APP_CFG_TASK_START_STK_SIZE-1],
    APP_TASK_MP3_MAIN_PRIO
    );

// Power up the MP3 streaming task
mp3_strm_pwrp();

} /* MP3_pwrp() */

/**
    Is a MP3 playback in progress

    @return Returns true if a playback is
            is in progress else returns
            FALSE.
*/
BOOLEAN MP3_playback_is_plybk_in_prog
    ( void )
{
BOOLEAN                 success;
MP3_playback_sts_type   sts;

sts = get_playback_status();

success = false;

if( ( MP3_PLAYBACK_STS_INIT         == sts ) ||
    ( MP3_PLAYBACK_STS_IN_PROGRESS  == sts )
  )
    {
    success = true;
    }

return success;
} /* MP3_playback_is_plybk_in_prog() */

/**
    Get the current playback status

    @return Returns the current playback status
*/
MP3_playback_sts_type MP3_playback_get_status
    ( void )
{

return get_playback_status();

} /* MP3_playback_get_status() */

/**
    Start an MP3 playback

    This function is used to start playback.
    The file should be loaded prior to calling
    this function.

    @return Returns if the playbackw was started
            successfully else returns false.
*/
BOOLEAN MP3_playback_start
    (
    const char* ptr_file_name
    )
{
BOOLEAN     success;
INT8U       err;

success = ( ptr_file_name != NULL );

if( success )
    {
    if( is_file_loaded() )
        {
        OSSemPend( intf_smphr_mp3, 0, &err );
        if( 0 != strncmp( ptr_file_name, cur_mp3_plbk_fname, MP3_PLAYBACK_FILE_NAME_LEN_MAX ) )
            {
            success = false;
            }
        OSSemPost( intf_smphr_mp3 );
        }
    else
        {
        success = send_msg( ptr_file_name );
        }

    if( success )
        {
        set_playback_status( MP3_PLAYBACK_STS_INIT );
        send_evnt( EVNT_PLAY_START );
        }
    }

return success;

} /* MP3_playback_start() */

/**
    Stop an MP3 playback

    This function is used to stop an ongoing
    playback.

    @return Returns if the playback was stopped
            successfully else returns false.
*/
BOOLEAN MP3_playback_stop
    ( void )
{
BOOLEAN success;

success = false;

if( MP3_PLAYBACK_STS_OFF  != get_playback_status() )
    {
    send_evnt( EVNT_PLAY_STOP );
    success = true;
    }

return success;

} /* MP3_playback_stop() */

/**
    Pause an MP3 playback

    This function is used to pause an ongoing
    playback.

    @return Returns if the playback was paused
            successfully else returns false.
*/

BOOLEAN MP3_playback_pause
    ( void )
{
BOOLEAN                 success;
MP3_playback_sts_type   sts;

sts = get_playback_status();

success = false;

if( ( MP3_PLAYBACK_STS_INIT         == sts ) ||
    ( MP3_PLAYBACK_STS_IN_PROGRESS  == sts )
  )
    {
    send_evnt( EVNT_PLAY_PAUSE );
    success = true;
    }

return success;

} /* MP3_playback_pause() */

/**
    Resume a paused MP3 playback

    This function is used to resume a paused
    playback.

    @return Returns if the playback was resume
            successfully else returns false.
*/
BOOLEAN MP3_playback_resume
    ( void )
{
BOOLEAN                 success;
MP3_playback_sts_type   sts;

sts = get_playback_status();

success = false;

if( MP3_PLAYBACK_STS_PAUSE == sts )
    {
    send_evnt( EVNT_PLAY_RESUME );
    success = true;
    }

return success;
} /* MP3_playback_resume() */

/**
    Get the playback time in seconds

    @return Returns the playback time in
            seconds
*/
INT16U MP3_playback_get_time_scnds
    ( void )
{

if( MP3_PLAYBACK_STS_OFF != get_playback_status() )
    {
    return mp3_strm_get_decode_time();
    }
else
    {
    return 0;
    }

} /* MP3_playback_get_time_scnds() */

/**
    MP3 main thread

    This function is waiting to receive various
    events associated with an MP3 playback
*/
void mp3_main
    (
    void* pdata
    )
{
OS_FLAGS rx_flags;

for(;;)
    {
    // Wait for an event
    rx_flags = wait_for_evnt();

    if( rx_flags & EVNT_RX_MSG )
        {
        INT8U       err;
        msg_type*   ptr_rx_msg;
        ptr_rx_msg = (msg_type*)OSMboxPend( main_mp3_msg_box, 0, &err );
        if( ptr_rx_msg != NULL )
            {
            set_playback_fname( ptr_rx_msg->fname );
            }
        }

    // Handle play start evnt
    if( rx_flags & EVNT_PLAY_START )
        {
        if( start_playback() )
            {
            set_playback_status( MP3_PLAYBACK_STS_IN_PROGRESS );
            mp3_strm_open();
            }
        else
            {
            stop_playback();
            }
        }

    // Handle play stop evnt
    if( rx_flags & EVNT_PLAY_STOP )
        {
        rx_flags &= ~EVNT_BUFFER_EMPTY;
        stop_playback();
        }

    // Handle a pause event
    if( rx_flags & EVNT_PLAY_PAUSE )
        {
        set_playback_status( MP3_PLAYBACK_STS_PAUSE );
        mp3_strm_pause();
        rx_flags &= ~EVNT_BUFFER_EMPTY;
        }

    // Handle a resume event
    if( rx_flags & EVNT_PLAY_RESUME )
        {
        set_playback_status( MP3_PLAYBACK_STS_IN_PROGRESS );
        if( !mp3_strm_resume() )
            {
            rx_flags |= EVNT_BUFFER_EMPTY;
            }
        }

    // Handle a buffer data event
    if( rx_flags & EVNT_BUFFER_EMPTY )
        {
        INT32U   size;
        size = 0;
        if( add_data_to_buffer( &size ) )
            {
            mp3_strm_write_data( strm_buff, size );
            }
        else
            {
            mp3_strm_close();
            set_playback_status( MP3_PLAYBACK_STS_DONE );
            }
        }
    }

}  /* mp3_main() */

/**
    Singal that the buffer is empty and request
    mode data

    This function is called by the streaming thread
    to request the MP3 main thread to read more data
    fromt the MP3 file and give it to the streaming
    thread for playback
*/
void mp3_signal_buffer_empty
    (
    void
    )
{
//Only if playback is in progress
if( MP3_PLAYBACK_STS_IN_PROGRESS  == get_playback_status() )
    {
    send_evnt( EVNT_BUFFER_EMPTY );
    }

} /* mp3_signal_buffer_empty() */

/**
    Send an event to the MP3 main thread

    Sets the required event bit in the event flags
*/
static void send_evnt
    (
    OS_FLAGS        evnt_flags
    )
{

INT8U err;
OSFlagPost( rx_events_mp3, evnt_flags, OS_FLAG_SET, &err );

}

/**
    Wait for an event

    Waits for an event flag to be set

    @return returns the flags

*/
static OS_FLAGS wait_for_evnt
    ( void )
{
INT8U       err;
OS_FLAGS    flags;

OSFlagPend( rx_events_mp3, 0xFF, OS_FLAG_WAIT_SET_ANY, 0, &err );

flags = rx_events_mp3->OSFlagFlags;

OSFlagPost( rx_events_mp3, 0xFF, OS_FLAG_CLR, &err );

return flags;

} /* wait_for_evnt(0 */


/**
    Get the playback status

    @return returns the playback status
*/
static MP3_playback_sts_type get_playback_status
    ( void )
{
INT8U                   err;
MP3_playback_sts_type   sts;

OSSemPend( intf_smphr_mp3, 0, &err );

sts = wksp_mp3.cur_playback_status;

OSSemPost( intf_smphr_mp3 );

return sts;
} /* get_playback_status() */

/**
    Set the playback file name

    Save the playback file's name to
    ram for playback
*/
static void set_playback_fname
    (
    const char* ptr_file_name
    )
{
INT8U                   err;

OSSemPend( intf_smphr_mp3, 0, &err );

strncpy( cur_mp3_plbk_fname, ptr_file_name, MP3_PLAYBACK_FILE_NAME_LEN_MAX );

OSSemPost( intf_smphr_mp3 );
} /* set_playback_fname() */

/**
    Is the file loaded

    @return returns true if the file was
    loaded successfully else returns false.
*/

static BOOLEAN is_file_loaded
    (
    void
    )
{
INT8U                   err;
BOOLEAN                 ret;

OSSemPend( intf_smphr_mp3, 0, &err );

ret = wksp_mp3.file_hndl_valid;

OSSemPost( intf_smphr_mp3 );

return ret;
}

/**
    Set the playback status
*/
static void set_playback_status
    (
    MP3_playback_sts_type sts
    )
{
INT8U err;

OSSemPend( intf_smphr_mp3, 0, &err );

wksp_mp3.cur_playback_status = sts;

OSSemPost( intf_smphr_mp3 );
} /* set_playback_status() */

/**
    Start playback

    This function is used to open the MP3 file
    on the SD card and seek to its beginning.
*/
static BOOLEAN start_playback
    ( void )
{
BOOLEAN success;
INT8U err;

OSSemPend( intf_smphr_mp3, 0, &err );

success = false;

if( !wksp_mp3.file_hndl_valid )
    {
    wksp_mp3.file_hndl = SD.open( cur_mp3_plbk_fname, O_READ );

    if( wksp_mp3.file_hndl.size() > 0 )
        {
        wksp_mp3.file_hndl.seek( 0 );
        wksp_mp3.file_hndl_valid = true;
        success = true;
        }
    else
        {
        wksp_mp3.file_hndl.close();
        wksp_mp3.file_hndl_valid = false;
        }
    }
else
    {
    wksp_mp3.file_hndl.seek( 0 );
    success = true;
    }

OSSemPost( intf_smphr_mp3 );

return success;
} /* start_playback() */

/**
    Start playback

    This function is used to stop an MP3 playback
*/

static void stop_playback
    ( void )
{
INT8U err;

mp3_strm_close();

OSSemPend( intf_smphr_mp3, 0, &err );

wksp_mp3.file_hndl.close();
wksp_mp3.file_hndl_valid = false;

OSSemPost( intf_smphr_mp3 );

set_playback_status( MP3_PLAYBACK_STS_OFF );


} /* stop_playback() */

/**
    Add data to buffer

    This function is used to read data from the MP3 file
    and copy it to a buffer so that the buffering thread
    can stream it to MP3 decoder.
*/
static BOOLEAN add_data_to_buffer
    (
    INT32U* ptr_size
    )
{
*ptr_size   = 0;

if( wksp_mp3.file_hndl.available() )
    {
    INT32U len;
    len         = wksp_mp3.file_hndl.read( strm_buff, 64 );
    *ptr_size   = len;
    }

return  ( ( *ptr_size > 0 ) ? true : false );
}

/**
    Send Message

    This function is used send the name of the playback
    file name as a message to the MP3 main thread.

    @return returns TRUE if the message was sent
    successfully else returns false.
*/

static BOOLEAN send_msg
    (
    const char* ptr_file_name
    )
{
msg_type    msg;
BOOLEAN     success;
INT8U       err;

success = false;

if( strlen( ptr_file_name ) <= MP3_PLAYBACK_FILE_NAME_LEN_MAX  )
    {
    strcpy( msg.fname, ptr_file_name );
    OSSemPend( intf_smphr_mp3, 0, &err );
    if( OS_ERR_NONE == OSMboxPost( main_mp3_msg_box, (void*)&msg ) )
        {
        send_evnt( EVNT_RX_MSG );
        success = true;
        }

    OSSemPost( intf_smphr_mp3 );
    }

return success;

} /* send_msg() */
