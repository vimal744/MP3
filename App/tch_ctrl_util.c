/*
    tch_ctrl_util.c
    Some utility functions for controlling the touch controller

    Developed for University of Washington embedded systems programming certificate

    2016/2 Nick Strathy wrote/arranged it
*/

#include "bsp.h"
#include "pjdf.h"
#include "pjdfCtrlTchCtrlFT6206.h"

/**
    Start the touch controller
*/
HANDLE tch_ctrl_util_start
    (
    void
    )
{
    // Open handle to the LCD driver
    HANDLE tch_ctrl_hndl = Open(PJDF_DEVICE_ID_TCHCTRL_FT6206, 0);

    if (!PJDF_IS_VALID_HANDLE(tch_ctrl_hndl)) while(1);

    return tch_ctrl_hndl;
}

/**
    Stop the touch controller
*/

void tch_ctrl_util_stop
    (
    HANDLE tch_ctrl_hndl
    )
{
    if (!PJDF_IS_VALID_HANDLE(tch_ctrl_hndl)) while (1);
}

/**
    Is a touch detected

    Returns a true if a touch is detected else returns a false

    Reference: The logic for this function is used from the ADAFRUIT FT6206
               library at https://github.com/adafruit/Adafruit_FT6206_Library
*/

BOOLEAN tch_ctrl_is_touched_detected
    (
    HANDLE tch_ctrl_hndl
    )
{
    uint8_t buffer[2];
    INT32U  bufLen;
    BOOLEAN success;
    OS_CPU_SR cpu_sr = 0;

    success = false;

    if (!PJDF_IS_VALID_HANDLE(tch_ctrl_hndl)) while (1);

    OS_ENTER_CRITICAL();

    buffer[0]   = PJDF_CTRL_TCHCTRL_FT6206_REG_NUMTOUCHES;
    bufLen      = 1;

    Read(tch_ctrl_hndl, (void*)buffer, &bufLen);

    if( ( buffer[0] == 1 ) || ( buffer[0] == 2 ) )
    {
        success = true;
    }

    OS_EXIT_CRITICAL();

    return success;
}

/**
    Determine the touch coordinates

    Reference: The logic for this function is used from the ADAFRUIT FT6206
               library at https://github.com/adafruit/Adafruit_FT6206_Library
*/

BOOLEAN tch_ctrl_get_touch_coodinates
    (
    uint16_t*   x,
    uint16_t*   y,
    HANDLE      tch_ctrl_hndl
    )
{
    BOOLEAN success;
    uint8_t buffer[16];
    INT32U  bufLen;
    uint8_t i;
    uint16_t touch_x[2], touch_y[2];

    OS_CPU_SR cpu_sr = 0;

    success     = false;
    *x          = 0;
    *y          = 0;
    buffer[0]   = 0;
    bufLen      = 15;

    if (!PJDF_IS_VALID_HANDLE(tch_ctrl_hndl)) while (1);

    OS_ENTER_CRITICAL();

    Read(tch_ctrl_hndl, (void*)buffer, &bufLen);

    OS_EXIT_CRITICAL();

    if( ( buffer[2] == 1 ) || ( buffer[2] == 2 ) )
    {
        for( i = 0; i < 2; i++ )
        {
            touch_x[i] = buffer[0x03 + i*6] & 0x0F;
            touch_x[i] <<= 8;
            touch_x[i] |= buffer[0x04 + i*6];
            touch_y[i] = buffer[0x05 + i*6] & 0x0F;
            touch_y[i] <<= 8;
            touch_y[i] |= buffer[0x06 + i*6];
        }

        *x = touch_x[0];
        *y = touch_y[0];
        success = true;
    }

    return success;

}
