/************************************************************************************

Copyright (c) 2001-2016  University of Washington Extension.

Module Name:

    tasks.c

Module Description:

    The tasks that are executed by the test application.

2016/2 Nick Strathy adapted it for NUCLEO-F401RE

************************************************************************************/
#include <stdarg.h>

#include "bsp.h"
#include "print.h"
#include "MP3_pub.h"
#include "TSK_pub.h"
#include "DFS_pub.h"
#include "SD.h"
#include "tch_ctrl_prv_util.h"

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ILI9341.h>


/**
    Literal Constants
*/

// Button Specifications
#define BUTTON_WIDTH            ( 44 )
#define BUTTON_HEIGHT           ( 40 )
#define BUTTON_X_SPACING        ( 14 )
#define FIRST_BUTTON_X          ( ILI9341_TFTWIDTH - 210                    )
#define FIRST_BUTTON_Y          ( ILI9341_TFTHEIGHT -20 - BUTTON_HEIGHT - 2 )

// Play back time info specifications
#define TIME_INFO_X             ( 10 )
#define TIME_INFO_Y             ( FIRST_BUTTON_Y - BUTTON_HEIGHT - 30 )
#define BOXSIZE                 ( 40 )

// File list specifications
#define LIST_SPACING            ( 30 )
#define LIST_BTN_WDT            ( ILI9341_TFTWIDTH - 2 )
#define LIST_BTN_HGT            ( 22 )
#define LIST_START_X            ( 2 )
#define LIST_START_Y            ( 50 )

#define PLAYBACK_FNAME_LEN_MAX  ( 15 )


/**
    Types
*/

// Enumeration for the various buttons
typedef uint8_t plybk_btn_type; enum
    {
    BTN_TYPE_PREV = 0,
    BTN_TYPE_PLAY = 1,
    BTN_TYPE_STOP = 2,
    BTN_TYPE_NEXT = 3,

    BTN_TYPE_CNT
    };

/**
    Memory constants
*/

// String for each button
char* s_Button_Name[BTN_TYPE_CNT] = {
                                      "Prev",
                                      "Play",
                                      "Stop",
                                      "Next",
                                    };


/**
    Static Variables
*/
static Adafruit_ILI9341     lcd_ctrl = Adafruit_ILI9341();
static Adafruit_GFX_List    file_list = Adafruit_GFX_List();

static Adafruit_GFX_Button  button_arr[BTN_TYPE_CNT];
static OS_STK               lcd_touch_task_stk[APP_CFG_TASK_START_STK_SIZE];
static HANDLE               hndl_tch_ctrl;
static INT16U               last_playback_time;
static INT32U               prev_touch_time;
static INT32U               cur_touch_time;
static INT16S               prev_sel_file_idx;

// Useful functions
void PrintWithBuf(char *buf, int size, char *format, ...);

void PrintToLcdWithBuf(char *buf, int size, char *format, ...);

static long MapTouchToScreen
    (
    long x,
    long in_min,
    long in_max,
    long out_min,
    long out_max
    );

static BOOLEAN populate_playback_file_list
    (
    void
    );

static void handle_selected_file_list_index
    (
    INT8S index
    );

static void handle_btn_press
    (
    INT16S   p_x,
    INT16S   p_y,
    INT32U   cur_touch_time
    );

static boolean get_pressed_button
    (
    plybk_btn_type*     out_btn,
    INT16S              x,
    INT16S              y
    );

static void main_lcd_touch
    (
    void* pdata
    );

static void draw_lcd_contents
    ( void );

/************************************************************************************

   Allocate the stacks for each task.
   The maximum number of tasks the application can have is defined by OS_MAX_TASKS in os_cfg.h

************************************************************************************/

void StartupTask(void* pdata)
{

    // Initialize the statics
    prev_touch_time = 0;
    cur_touch_time = 0;
    prev_sel_file_idx = -1;

    // Start the system tick
    OS_CPU_SysTickInit(OS_TICKS_PER_SEC);

    // Power up the devices's file system
    DFS_pwrp();

    // Init the device's file system
    DFS_init();

    // Power up the MP3 main thread
    MP3_pwrp();

    // Create the LCD and Touch main
    OSTaskCreate( main_lcd_touch, (void*)0, &lcd_touch_task_stk[APP_CFG_TASK_START_STK_SIZE-1], APP_TASK_LCD_TOUCH_PRIO);

    // Delete the startup task
    OSTaskDel(OS_PRIO_SELF);
}

/**
    Draw the lcd contents

    This function draws the front
    end of the MP3 application

*/
static void draw_lcd_contents
    ( void )
{
    char    buf[15];
    uint8_t i;

    // Clear the screen
    lcd_ctrl.fillScreen(ILI9341_BLACK);

    // Draw header
    lcd_ctrl.setCursor(60, 20);
    lcd_ctrl.setTextColor(ILI9341_WHITE);
    lcd_ctrl.setTextSize(2);
    PrintToLcdWithBuf(buf, 15, "MP3 Player");

    // Draw the various play control buttons
    for(  i = 0; i < BTN_TYPE_CNT; i++ )
    {
        button_arr[i].drawButton(); // display button
    }

    // Populate and draw the playback list
    populate_playback_file_list();

    file_list.DrawList();

} /* draw_lcd_contents() */

/**
    Main thread for handling the interaction between
    the User and the MP3 player

    This thread is continuously polling for an LCD touch.

    Depending on the control pressed on the screen, the
    necessary action is taken

*/
static void main_lcd_touch
    (
    void* pdata
    )
{
    PjdfErrCode pjdfErr;
    INT32U      length;
    uint8_t     i;
    INT16S     btn_x;

    // Open handle to the LCD driver
    HANDLE lcd_hndl = Open( PJDF_DEVICE_ID_LCD_ILI9341, 0 );
    if (!PJDF_IS_VALID_HANDLE(lcd_hndl)) while(1);

    // We talk to the LCD controller over a SPI interface therefore
    // open an instance of that SPI driver and pass the handle to
    // the LCD driver.
    HANDLE spi_hndl = Open( LCD_SPI_DEVICE_ID, 0 );
    if (!PJDF_IS_VALID_HANDLE(spi_hndl)) while(1);

    length = sizeof(HANDLE);
    pjdfErr = Ioctl(lcd_hndl, PJDF_CTRL_LCD_SET_SPI_HANDLE, &spi_hndl, &length);
    if(PJDF_IS_ERROR(pjdfErr)) while(1);

    lcd_ctrl.setPjdfHandle(lcd_hndl);
    lcd_ctrl.begin();

    // Initialize the playback control buttons
    btn_x = FIRST_BUTTON_X;
    for(  i = 0; i < BTN_TYPE_CNT; i++ )
    {
        button_arr[i] = Adafruit_GFX_Button();

        button_arr[i].initButton( &lcd_ctrl,
                                     btn_x,
                                     FIRST_BUTTON_Y,
                                     BUTTON_WIDTH,
                                     BUTTON_HEIGHT,
                                     ILI9341_YELLOW,
                                     ILI9341_BLACK,
                                     ILI9341_YELLOW,
                                     s_Button_Name[i],
                                     1 );

        btn_x += ( BUTTON_WIDTH + BUTTON_X_SPACING );
    }


    // init the playback list
    file_list.InitList( &lcd_ctrl, LIST_START_X, LIST_START_Y, LIST_BTN_WDT, LIST_BTN_HGT );

    // Draw the MP3 User interface
    draw_lcd_contents();

    // Start the touch control
    hndl_tch_ctrl = tch_ctrl_util_start();

    while (1)
    {
        boolean     touched;
        uint16_t    x;
        uint16_t    y;
        int         len;
        char        temp_buff[12];
        char        buf[12];

        // Is a touch detected
        touched = tch_ctrl_is_touched_detected( hndl_tch_ctrl );

        // Save the CPU touch at the time of touch
        if( touched )
        {
            cur_touch_time = task_ms_timer;
        }

        // If there is change in playback time, update the
        // playback time on the screen
        if( MP3_playback_get_time_scnds() != last_playback_time )
        {
            INT16U mins = MP3_playback_get_time_scnds()/ 60;

            last_playback_time = MP3_playback_get_time_scnds();

            len=snprintf( temp_buff, 12, "%02u:%02u", mins, ( MP3_playback_get_time_scnds() % 60 ) );

            lcd_ctrl.fillRect(0, TIME_INFO_Y, ILI9341_TFTWIDTH-10, BOXSIZE, ILI9341_BLACK);

            if( len > 0 )
            {
                // Print a message on the LCD
                lcd_ctrl.setCursor( TIME_INFO_X, TIME_INFO_Y + 5 );
                lcd_ctrl.setTextColor(ILI9341_WHITE);
                lcd_ctrl.setTextSize(2);
                PrintToLcdWithBuf(buf, 15, temp_buff);
            }

        }

        // If plaback is in progress
        if( MP3_playback_is_plybk_in_prog() )
        {
            // Change play button to pause
            button_arr[BTN_TYPE_PLAY].updateText("Pause");
        }
        else if( MP3_PLAYBACK_STS_DONE == MP3_playback_get_status() )
        {
            // Go to the next item on the plaback list and
            // play it
            handle_selected_file_list_index( file_list.GetNextIndex() );
        }
        else
        {
            button_arr[BTN_TYPE_PLAY].updateText("Play");
        }

        if( !touched )
        {
            OSTimeDly(2);
            continue;
        }

        // If touch was detected, determine the touch co-ordinates
        if( tch_ctrl_get_touch_coodinates( &x, &y, hndl_tch_ctrl ) )
        {
            x = (INT16S)MapTouchToScreen( x, 0, ILI9341_TFTWIDTH, ILI9341_TFTWIDTH, 0   );
            y = (INT16S)MapTouchToScreen( y, 0, ILI9341_TFTHEIGHT, ILI9341_TFTHEIGHT, 0 );
        }
        else
        {
            touched = false;
        }

        if( touched  )
        {
            // Handle a file list item press
            handle_selected_file_list_index( file_list.GetSelectedIndex( x, y ) );

            // Handle a button press
            handle_btn_press( x, y, cur_touch_time );
        }

        OSTimeDly(2);
    }
}

/**
    Renders a character at the current cursor
    position on the LCD
*/
static void PrintCharToLcd(char c)
{
    lcd_ctrl.write(c);
}

/**
    Print a formated string with the given
    buffer to LCD.

*/
void PrintToLcdWithBuf(char *buf, int size, char *format, ...)
{
    va_list args;
    va_start(args, format);
    PrintToDeviceWithBuf(PrintCharToLcd, buf, size, format, args);
    va_end(args);
}

/**
    Function to determine which button was pressed
    based on the co-ordinates

*/
static boolean get_pressed_button
    (
    plybk_btn_type* out_btn,
    INT16S          x,
    INT16S          y
    )
{
    boolean success;
    uint8_t i;

    success = false;

    for( i = 0; i < BTN_TYPE_CNT; i++ )
    {
        if( button_arr[i].contains( x , y ) )
        {
            *out_btn = (plybk_btn_type)i;
            success = true;
            break;
        }
    }

    return success;

} /* get_pressed_button() */

/**
    Function to populate the playback file list

    This function get the names of all the files
    present in the root the SD card.

    Limitations:
        1) All the files exist in the root of the SD card
        2) Due to code size constraints, upto a maximum of only 4 files
           will be added to the list
*/

static BOOLEAN populate_playback_file_list
    (
    void
    )
{
    File    root;
    File    entry;
    BOOLEAN files_added;

    // Open the root of the SD card drive
    root = SD.open("/");
    files_added = false;

    // Iterate and get the next file
    while(true)
    {
        // Get the next file
        File entry =  root.openNextFile();

        if (!entry )
        {
            break;
        }

        // If the entry is a file
        if( !entry.isDirectory() )
        {
            // Add the file name to the list
            if( !file_list.AddItem( entry.name() ) )
            {
                entry.close();
                break;
            }
            else
            {
                files_added = true;
            }
        }

        entry.close();

    }

    root.seek( 0 );

    return files_added;

} /* populate_playback_file_list()*/

/**
    Function to handle a file list selection

    If a valid file exists for a give file list item,
    this function will select it and start playing it
    back.

    If playback is already in progress, this function
    will stop an ongoing playback and start playing back
    the new selection
*/

static void handle_selected_file_list_index
    (
    INT8S index
    )
{
    // Make sure the index is valid and has really changed
    if( ( -1 !=  index ) && ( prev_sel_file_idx != index ) )
    {
        // Get the string assocaited with the list item
        const char* ptr_fname = file_list.GetText( index );

        if( ptr_fname != NULL )
        {
            // If playback is not OFF, stop existing playback
            if( MP3_PLAYBACK_STS_OFF != MP3_playback_get_status() )
            {
                MP3_playback_stop();
                OSTimeDly(500);
            }

            // Update the file list selection
            file_list.SetSelectedIndex( index );
            prev_sel_file_idx = index;

            // Start playback with the new selection
            if( !MP3_playback_start( ptr_fname ) )
            {
                while(1);
            }
        }
    }

} /* handle_selected_file_list_index() */

/**
    Function to handle playback control button press

    This function is used to take an action based on
    the playback control button that was pressed
*/

static void handle_btn_press
    (
    INT16S  p_x,
    INT16S  p_y,
    INT32U   cur_touch_time
    )
{
    plybk_btn_type  buttonPressed;

    // Make sure to filter duplicate button presses
    if( ( ( cur_touch_time - prev_touch_time ) > 300 ) &&
        ( get_pressed_button( &buttonPressed, p_x, p_y ) )
      )
    {
        // If the playback button was pressed
        if( BTN_TYPE_PLAY == buttonPressed )
        {
            // If playback is in progress
            if( MP3_playback_is_plybk_in_prog() )
            {
                // pause playback
                MP3_playback_pause();
            }
            else
            {
                // If playback was paused
                if( MP3_PLAYBACK_STS_PAUSE == MP3_playback_get_status() )
                {
                    // Resume playback
                    MP3_playback_resume();
                }
                else
                {
                    // Start playback
                    MP3_playback_start( file_list.GetText( prev_sel_file_idx ) );
                }
            }
        }
        // If the stop button was pressed
        else if( BTN_TYPE_STOP == buttonPressed )
        {
            // Stop ongoing playback
            MP3_playback_stop();
            OSTimeDly(500);

            // Reset the file list selection
            file_list.SetSelectedIndex(-1);
            prev_sel_file_idx = -1;
        }
        // If the next button was pressed
        else if( BTN_TYPE_NEXT == buttonPressed )
        {
            // Change the file list selection to the next list item
            handle_selected_file_list_index( file_list.GetNextIndex() );
        }
        // If the previous button was pressed
        else if( BTN_TYPE_PREV == buttonPressed )
        {
            // Change the file list selection to the previous list item
            handle_selected_file_list_index( file_list.GetPrevIndex() );
        }

        prev_touch_time = cur_touch_time;
    }

} /* handle_btn_press() */

/**
    Translate the touch control co-ordinates to
    lcd co-ordiates
*/

static long MapTouchToScreen
    (
    long x,
    long in_min,
    long in_max,
    long out_min,
    long out_max
    )
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
