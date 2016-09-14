/*
    tch_ctrl_prv_util.h
    Private declaration for the touch controller
*/

#ifndef TCH_CTRL_PRV_H
#define TCH_CTRL_PRV_H

#include "bsp.h"

/*---------------------------------
tch_ctrl_prv_util.c
---------------------------------*/

HANDLE tch_ctrl_util_start
    (
    void
    );

void tch_ctrl_util_stop
    (
    HANDLE hTchCtrl
    );

BOOLEAN tch_ctrl_is_touched_detected
    (
    HANDLE hTchCtrl
    );

BOOLEAN tch_ctrl_get_touch_coodinates
    (
    uint16_t*   x,
    uint16_t*   y,
    HANDLE      hTchCtrl
    );


#endif // TCH_CTRL_PRV_H
