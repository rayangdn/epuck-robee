/**
 * @file    TOF_sensor.c
 * @brief   Handles the TOF sensor of the robot.
**/

//ChibiOS headers
#include <ch.h>
#include <hal.h>

//E-puck 2 headers
#include <sensors/VL53L0X/VL53L0X.h>

//Project headers

#include "include/TOF_sensor.h"

/*===========================================================================*/
/* File exported functions.                                                  */
/*===========================================================================*/

uint16_t get_TOF_value(void)
{
    return VL53L0X_get_dist_mm();
}

void sensor_start(void)
{
    VL53L0X_start();
}