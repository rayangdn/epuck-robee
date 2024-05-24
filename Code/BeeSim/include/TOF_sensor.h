/**
 * @file	TOF_sensor.h
 * @brief	Exported functions and constants related to
 * 			the sensors of the robot.
**/

#ifndef SENSORS_H
#define SENSORS_H

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/**
 * @brief               gets the value of the TOF sensor
 * @return              value of the TOF sensor
**/
uint16_t get_TOF_value(void);
/**
 * @brief               starts VL53L0X sensor threads
 * @return              none
 */
void sensor_start(void);

#endif /* SENSORS_H */