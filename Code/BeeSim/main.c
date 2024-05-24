/**
 * @file    main.c
 * @brief   Main file, handles interactions between source files.
**/

// ChibiOS headers
#include <ch.h>
#include <hal.h>
#include <memory_protection.h>
#include <spi_comm.h>

// Project headers
#include "main.h"
#include "include/process_audio.h"
#include "include/process_image.h"
#include "include/controller.h"
#include "include/TOF_sensor.h"


/*===========================================================================*/
/* Local functions.                                                          */
/*===========================================================================*/

static void init_all(void)
{
 	halInit();
    chSysInit();
    mpu_init();	

	//starts the rgb LEDs
    spi_comm_start();

	//stars the threads for the audio processing, the image processing,
	// the controller and the  TOF sensor
	process_audio_start();
	process_image_start();
	controller_start();
	sensor_start();
}

/*===========================================================================*/
/* Main function.                                                   		 */
/*===========================================================================*/

int main(void)
{
	init_all();
	while(1)
	{
		//does nothing
		chThdSleepMilliseconds(10000);
	}
}

#define STACK_CHK_GUARD 0xe2dee396
uintptr_t __stack_chk_guard = STACK_CHK_GUARD;

void __stack_chk_fail(void)
{
    chSysHalt("Stack smashing detected");
}