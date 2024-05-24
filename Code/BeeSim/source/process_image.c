/**
 * @file    process_image.c
 * @brief   Handles the capture and the processing of an image.
 * @note    Inspired by  TP4.  
**/

//C headers
#include <stdlib.h>

//ChibiOS headers
#include <ch.h>
#include <hal.h>

// E-puck 2 headers
#include <camera/dcmi_camera.h>
#include <camera/po8030.h>

//Project headers
#include "include/process_image.h"
#include "include/process_audio.h"

/*===========================================================================*/
/* File constants.                                                           */
/*===========================================================================*/

//lines used for the detection, inside [0...478]
#define USED_LINE			    200

//image processing constants
#define DETECTION_THRESHOLD     20
#define WIDTH_SLOPE		        30
#define MIN_BALLOON_WIDTH		50
#define TOO_CLOSE_TO_BALLOON    400
#define COEFF_RANGE_CAPTURE     1

/*===========================================================================*/
/* File local variables.                                                     */
/*===========================================================================*/

static bool capture_image = true;
static uint16_t balloon_position = IMAGE_BUFFER_SIZE/2;
static uint8_t balloon_type = NONE;

/*===========================================================================*/
/* Semaphores.                                                               */
/*===========================================================================*/

static BSEMAPHORE_DECL(image_ready_sem, TRUE);

/*===========================================================================*/
/* File local functions.                                                     */
/*===========================================================================*/

/**
 * @brief               Detects if the beginning of a balloon is in the image.
 * @param[in]   image   the image to process
 * @param[in]   i       the index of the pixel to process
 * @return              i, the index of the beginning of the balloon
**/
static uint16_t detect_beginning(uint8_t* image, uint16_t i)
{
	if(abs(image[i] - image[i+WIDTH_SLOPE]) > DETECTION_THRESHOLD)
	{
		//we check if we have detected a flower or an ennemy
		if(image[i] > image[i+WIDTH_SLOPE])
		{
			balloon_type = FLOWER;
			return i;
		} else if (image[i] < image[i+WIDTH_SLOPE]){
			balloon_type = ENNEMY;
			return i;
		}
	}
	return 0;
}

/**
 * @brief            	Detects if the ending of a balloon is in the image.
 * @param[in]   image   the image to process
 * @param[in]   i       the index of the pixel to process
 * @return              i, the index of the ending of the balloon
**/
static uint16_t detect_ending(uint8_t* image, uint16_t i)
{
	//checking if we previously have detected a flower or an ennemy
	if(balloon_type == FLOWER)
	{
		if(abs(image[i] - image[i-WIDTH_SLOPE]) > DETECTION_THRESHOLD && image[i-WIDTH_SLOPE] < image[i])
		{
			return i;
		}
	} else if (balloon_type == ENNEMY){
		if(abs(image[i] - image[i-WIDTH_SLOPE]) > DETECTION_THRESHOLD && image[i-WIDTH_SLOPE] > image[i])
		{
			return i;
		}
	}
	return 0;
}

/**
 * @brief               Detects a balloon in the image, set the line position.
 *                      and the balloon_detected variable
 * @param[in]   image   the image to process
 * @return              none
**/
static void detect_balloon(uint8_t* image)
{

	uint16_t i = 0, begin = 0, end = 0;
	uint8_t stop = 0, wrong_balloon = 0, balloon_not_found = 0;

		do 
	{
		wrong_balloon = 0;
		//search for a begin
		while(stop == 0 && i < IMAGE_BUFFER_SIZE - COEFF_RANGE_CAPTURE*WIDTH_SLOPE)
		{
            begin = detect_beginning(image, i);
            if(begin > 0)
            {
                stop = 1;
            }
            i++;
        } 
		//if a begin was found, search for an end
		if (i < IMAGE_BUFFER_SIZE - COEFF_RANGE_CAPTURE*WIDTH_SLOPE && begin)
		{
		    stop = 0;
		    while(stop == 0 && i < IMAGE_BUFFER_SIZE -COEFF_RANGE_CAPTURE*WIDTH_SLOPE)
		    {
                end = detect_ending(image, i);
                if(end > 0)
                {
                    stop = 1;
                } 
				i++;
		    }
		    //if an end was not found
		    if (i > IMAGE_BUFFER_SIZE - COEFF_RANGE_CAPTURE*WIDTH_SLOPE || !end)
		    {
		        balloon_not_found = 1;
		    }
		//if no begin was found
		} else {  
		    balloon_not_found = 1;
		}

		//if a line too small has been detected, continues the search
		if(!balloon_not_found && (end-begin) < MIN_BALLOON_WIDTH)
		{
			i = end;
			begin = 0;
			end = 0;
			stop = 0;
			wrong_balloon = 1;
		}
	} while(wrong_balloon);

	if(balloon_not_found)
	{
		//reset to default values
		begin = 0;
		end = 0;
		balloon_position = IMAGE_BUFFER_SIZE/2;
        balloon_type = NONE;
	} else {
		//gives the line position
		balloon_position = (begin + end)/2;

		//if we are close to the ballon, we don't want to capture image to avoid errors
		//the last few centimeters are handled by the TOF sensor
		if((end-begin) > TOO_CLOSE_TO_BALLOON)
		{
			capture_image = false;
			balloon_position = IMAGE_BUFFER_SIZE/2;
		}

	}
}

/*===========================================================================*/
/* File threads.                                                             */
/*===========================================================================*/

static THD_WORKING_AREA(waCaptureImage, 256);
static THD_FUNCTION(CaptureImage, arg) 
{
     chRegSetThreadName(__FUNCTION__);
    (void)arg;

	//takes pixels 0 to IMAGE_BUFFER_SIZE of the lines USED_LINE and USED_LINE + 1 
	po8030_advanced_config(FORMAT_RGB565, 0, USED_LINE, IMAGE_BUFFER_SIZE, 2, SUBSAMPLING_X1, SUBSAMPLING_X1);
	dcmi_enable_double_buffering();
	dcmi_set_capture_mode(CAPTURE_ONE_SHOT);
	dcmi_prepare();

    while(1){

		//waits for the mode to be MOVING_TO_BALLOON to starts the capture
		//avoid capturing images when not needed
		if(get_mode() == MOVING_TO_BALLOON && capture_image)
		{
			//starts a capture
			dcmi_capture_start();
			//waits for the capture to be done
			wait_image_ready();
			//signals an image has been captured
			chBSemSignal(&image_ready_sem);
		} else {
			balloon_position = IMAGE_BUFFER_SIZE/2;
			chThdSleepMilliseconds(200);
		}
    }
}

static THD_WORKING_AREA(waProcessImage, 1024);
static THD_FUNCTION(ProcessImage, arg) 
{
    chRegSetThreadName(__FUNCTION__);
    (void)arg;

	uint8_t *img_buff_ptr;
	uint8_t image[IMAGE_BUFFER_SIZE] = {0};

    while(1){
    	//waits until an image has been captured
        chBSemWait(&image_ready_sem);

		//gets the pointer to the array filled with the last image in RGB565    
		img_buff_ptr = dcmi_get_last_image_ptr();

		//extracts only the green pixels
		for(uint16_t i = 0 ; i < (2 * IMAGE_BUFFER_SIZE) ; i+=2){
			//extracts 3 LSbits of the first byte and the 3 MSbits of second byte
			image[i/2] = (((uint8_t)img_buff_ptr[i] & 0x07) << 5 )
						+ (((uint8_t)img_buff_ptr[i+1] & 0xE0) >> 3);
				
		}
		detect_balloon(image);
	}
}

/*===========================================================================*/
/* File exported functions.                                                  */
/*===========================================================================*/

uint16_t get_balloon_position(void){
	return balloon_position;
}

balloon_type_t get_balloon_type(void)
{
	return balloon_type;
}

void set_capture_image(bool capture)
{
	capture_image = capture;
}

void process_image_start(void)
{
    //starts the camera
    dcmi_start();
	po8030_start();
    //starts the threads for the capture and processing of the image
	chThdCreateStatic(waCaptureImage, sizeof(waCaptureImage), NORMALPRIO+1, CaptureImage, NULL);
	chThdCreateStatic(waProcessImage, sizeof(waProcessImage), NORMALPRIO+1, ProcessImage, NULL);

}