/**
    @file        dfs_main.c

    @author      Vimal Mehta

    @description
        This module is to control the device's file system.
    Currently this is only targeted towards a SD card backend

    Copyright (c) 2016 Vimal Mehta
*/

#include <stdarg.h>
#include "DFS_pub.h"

#include "bsp.h"
#include "SD.h"

/**
    Types
*/

// Workspace type

typedef struct
    {
    HANDLE h_SD;
    HANDLE h_SPI;
    } dfs_ws_type;


/**
    Static Variables
*/

static dfs_ws_type  wksp_dfs;


/**
    Power up the Device file system

    This function is used to initialize all the
    global variables for the DFS module


    @return None
*/
void DFS_pwrp
    ( void )
{

wksp_dfs.h_SD   = 0;
wksp_dfs.h_SPI  = 0;

} /* DFS_pwrp() */

/**
    Initialize the Device's file system

    This function is used to init the
    file system by enabling the SD driver

    NOTE: The control will wait in an infinite
    while(1) loop if this funciton fails.

    @return None
*/

void DFS_init
    ( void )
{
PjdfErrCode pjdfErr;
INT32U length;

// Open handle to the SD driver the first time we stream a file
wksp_dfs.h_SD = Open( PJDF_DEVICE_ID_SD_ADAFRUIT, 0 );
if( !PJDF_IS_VALID_HANDLE( wksp_dfs.h_SD ) )
    {
    while(1);
    }

wksp_dfs.h_SPI = Open( SD_SPI_DEVICE_ID, 0 );
if( !PJDF_IS_VALID_HANDLE( wksp_dfs.h_SPI ) )
    {
    while(1);
    }

length = sizeof( HANDLE );
pjdfErr = Ioctl(  wksp_dfs.h_SD, PJDF_CTRL_SD_SET_SPI_HANDLE, &wksp_dfs.h_SPI, &length );
if( PJDF_IS_ERROR( pjdfErr ) )
    {
    while(1);
    }

if( !SD.begin( wksp_dfs.h_SD  ) )
    {
    while(1);
    }
} /* DFS_init() */
