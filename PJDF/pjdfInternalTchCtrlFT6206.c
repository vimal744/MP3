#include "bsp.h"
#include "pjdfInternal.h"
#include "pjdfCtrlTchCtrlFT6206.h"
#include "bspI2c.h"

// PJDF DEVELOPER TODO LIST FOR ADDING A NEW DRIVER:
//    - define a new PJDF_DEVICE_ID_<MYDEVICE> below
//    - reference it under PJDF_DEVICE_IDS below
//    - add a new pjdfInternal<mydevice>.c module to implement the pjdfInternal.h interface
//    - reference the Init() function of your driver in the driversInternal array in pjdf.c
//    - add a new pjdfCtrl<mydevice>.h interface to define the Ioctl() functionality of your device
//    - #include your pjdfCtrl<mydevice>.h in the present header file above
//    - add modules as needed to the BSP folder to keep board-dependent code out of your PJDF implementation

/**
    Function to read required amounts of bytes of data
    from a specified address of the touch controller
*/

static PjdfErrCode ReadTouchController(DriverInternal *pDriver, void* pBuffer, INT32U* pCount)
{
    uint8_t i;

    if( *pCount < 1 )
    {
        while(1);
    }

    // Send the start signal
    I2C_start( I2C1, PJDF_CTRL_TCHCTRL_FT6206_ADDR<<1, I2C_Direction_Transmitter );

    // Write the address which is the first buffer item
    I2C_write( I2C1, ((uint8_t*)pBuffer)[0] );

    // Send the stop signal
    I2C_stop( I2C1 );

    // Change to receiver mode
    I2C_start( I2C1, PJDF_CTRL_TCHCTRL_FT6206_ADDR<<1, I2C_Direction_Receiver );

    // Read required number of bytes acknowledging each one of them
    // except the last one
    for( i = 0; i < (*pCount - 1 ); i++ )
    {
        ((uint8_t*)pBuffer)[i] = (uint8_t)I2C_read_ack(I2C1);
    }
    ((uint8_t*)pBuffer)[i] = (uint8_t)I2C_read_nack(I2C1);

    return PJDF_ERR_NONE;
}

/**
    Function to write required amounts of bytes of data
    to a specified address of the touch controller

    NOTE: Currently only supports writing only one register
    at a time.

*/

static PjdfErrCode WriteTouchController(DriverInternal *pDriver, void* pBuffer, INT32U* pCount)
{
    if( *pCount != 2 )
    {
        while(1);
    }

    // Write the start condition
    I2C_start( I2C1, PJDF_CTRL_TCHCTRL_FT6206_ADDR<<1, I2C_Direction_Transmitter );

    // Write the register address
    I2C_write( I2C1, ((uint8_t*)pBuffer)[0] );

    // Write the data for the register
    I2C_write( I2C1, ((uint8_t*)pBuffer)[1] );

    // Write the stop condition
    I2C_stop( I2C1 );

    return PJDF_ERR_NONE;
}

/**
    Function to open the touch controller driver
*/

static PjdfErrCode OpenTouchController(DriverInternal *pDriver, INT8U flags)
{
    uint8_t  buffer[2];
    INT32U   buffLen;

    // Read the vendor ID
    buffer[0]  = PJDF_CTRL_TCHCTRL_FT6206_REG_VENDID;
    buffLen    = 1;
    ReadTouchController( pDriver, (void*)&buffer, &buffLen );

    // make sure it matches the expected value
    if( buffer[0] != PJDF_CTRL_TCHCTRL_FT6206_VENDOR_ID_VAL )
    {
        while(1);
    }

    // Read the chip id
    buffer[0]  = PJDF_CTRL_TCHCTRL_FT6206_REG_CHIPID;
    buffLen    = 1;
    ReadTouchController( pDriver, (void*)&buffer, &buffLen );

    // make sure it matches the expected value
    if( buffer[0] != PJDF_CTRL_TCHCTRL_FT6206_CHIP_ID_VAL )
    {
        while(1);
    }

    // Set the touch threshold
    buffer[0] = PJDF_CTRL_TCHCTRL_FT6206_REG_THRESHHOLD;
    buffer[1] = PJDF_CTRL_TCHCTRL_FT6206_DFLT_TCH_THRESHOLD;
    buffLen   = 2;
    WriteTouchController( pDriver, (void*)&buffer, &buffLen );

    return PJDF_ERR_NONE;
}


static PjdfErrCode CloseTouchController(DriverInternal *pDriver)
{
    return PJDF_ERR_NONE;
}


static PjdfErrCode IoctlTouchController(DriverInternal *pDriver, INT8U request, void* pArgs, INT32U* pSize)
{
    return PJDF_ERR_NONE;
}


// Initializes the given VS1053 MP3 driver.
PjdfErrCode InitTouchControllerFT6206(DriverInternal *pDriver, char *pName)
{
    if (strcmp (pName, pDriver->pName) != 0) while(1); // pName should have been initialized in driversInternal[] declaration

    // Initialize semaphore for serializing operations on the device
    pDriver->sem = OSSemCreate(1);
    if (pDriver->sem == NULL) while (1);  // not enough semaphores available
    pDriver->refCount = 0; // number of Open handles to the device
    pDriver->maxRefCount = 1; // only one open handle allowed
    pDriver->deviceContext = NULL;

    // Assign implemented functions to the interface pointers
    pDriver->Open = OpenTouchController;
    pDriver->Close = CloseTouchController;
    pDriver->Read = ReadTouchController;
    pDriver->Write = WriteTouchController;
    pDriver->Ioctl = IoctlTouchController;

    pDriver->initialized = OS_TRUE;

    I2C1_init();

    return PJDF_ERR_NONE;
}
