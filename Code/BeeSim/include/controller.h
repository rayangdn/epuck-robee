/**
 * @file	controller.h
 * @brief	Exported functions and constants related to
 * 			the control of the robot.
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

/*===========================================================================*/
/* File data structures and types.                                           */
/*===========================================================================*/

//attribute packed to use one byte only
typedef enum __attribute__((__packed__)) action_type_t
{
	SEARCHING, 
    APPROACHING, 
    POLLINATING,
    ATTACKING
} action_type_t;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/**
 * @brief   Initializes motors and starts controller thread.
 * @return  none
**/
void controller_start(void);

#endif /* CONTROLLER_H */