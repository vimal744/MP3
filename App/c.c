/*
    mp3Util.c
    Some utility functions for controlling the MP3 decoder.

    Developed for University of Washington embedded systems programming certificate

    2016/2 Nick Strathy wrote/arranged it
*/

#include "bsp.h"
#include "pjdf.h"
#include "pjdfCtrlTchCtrlFT6206.h"

/************************************************************************************
   tch_ctrl_util_start
************************************************************************************/

HANDLE tch_ctrl_util_start
    (
    void
    )
{
    // Open handle to the LCD driver
    HANDLE hTchCtrl = Open(PJDF_DEVICE_ID_TCHCTRL_FT6206, 0);

    if (!PJDF_IS_VALID_HANDLE(hTchCtrl)) while(1);

    return hTchCtrl;
}

/************************************************************************************
   tch_ctrl_util_stop
************************************************************************************/

void tch_ctrl_util_stop
    (
    HANDLE hTchCtrl
    )
{
    if (!PJDF_IS_VALID_HANDLE(hTchCtrl)) while (1);
}

/************************************************************************************
   tch_ctrl_is_touched

    Reference:
        The logic for this function is taken from the Adafruit FT6206  library
        at https://github.com/adafruit/Adafruit_FT6206_Library
************************************************************************************/

BOOLEAN tch_ctrl_is_touched_detected
    (
    HANDLE hTchCtrl
    )
{
    uint8_t buffer[2];
    INT32U  bufLen;
    BOOLEAN success;
    OS_CPU_SR cpu_sr = 0;

    success = false;

    if (!PJDF_IS_VALID_HANDLE(hTchCtrl)) while (1);

    OS_ENTER_CRITICAL();

    buffer[0]   = PJDF_CTRL_TCHCTRL_FT6206_REG_NUMTOUCHES;
    bufLen      = 1;

    Read(hTchCtrl, (void*)buffer, &bufLen);

    if( ( buffer[0] == 1 ) || ( buffer[0] == 2 ) )
    {
        success = true;
    }

    OS_EXIT_CRITICAL();

    return success;
}

/************************************************************************************
   tch_ctrl_get_touch_coodinates


    Reference:
        The logic for this function is taken from the Adafruit FT6206  library
        at https://github.com/adafruit/Adafruit_FT6206_Library
************************************************************************************/

BOOLEAN tch_ctrl_get_touch_coodinates
    (
    uint16_t*   x,
    uint16_t*   y,
    HANDLE      hTchCtrl
    )
{
    BOOLEAN success;
    uint8_t i2cdat[16];
    INT32U  bufLen;
    uint8_t i;
    uint16_t touch_x[2], touch_y[2];

    OS_CPU_SR cpu_sr = 0;

    success     = false;
    *x          = 0;
    *y          = 0;
    i2cdat[0]   = 0;
    bufLen      = 15;

    if (!PJDF_IS_VALID_HANDLE(hTchCtrl)) while (1);

    OS_ENTER_CRITICAL();

    Read(hTchCtrl, (void*)i2cdat, &bufLen);

    OS_EXIT_CRITICAL();

    if( ( i2cdat[2] == 1 ) || ( i2cdat[2] == 2 ) )
    {
        for( i = 0; i < 2; i++ )
        {
            touch_x[i] = i2cdat[0x03 + i*6] & 0x0F;
            touch_x[i] <<= 8;
            touch_x[i] |= i2cdat[0x04 + i*6];
            touch_y[i] = i2cdat[0x05 + i*6] & 0x0F;
            touch_y[i] <<= 8;
            touch_y[i] |= i2cdat[0x06 + i*6];
        }

        *x = touch_x[0];
        *y = touch_y[0];
        success = true;
    }

    return success;

}
