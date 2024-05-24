/**
 * @file	process_image.h
 * @brief	Exported functions and constants related to
 * 			image processing and capture.
**/

#ifndef PROCESS_IMAGE_H
#define PROCESS_IMAGE_H

/*===========================================================================*/
/* File constants.                                                           */
/*===========================================================================*/

//buffer size
#define IMAGE_BUFFER_SIZE   640

/*===========================================================================*/
/* File data structures and types.                                           */
/*===========================================================================*/

//attribute packed to use one byte only
typedef enum __attribute__((__packed__)) balloon_type_t
{
	NONE, 
    FLOWER, 
    ENNEMY
} balloon_type_t;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/
 
/**
 * @brief   returns balloon_position
**/
uint16_t get_balloon_position(void);

/**
 * @brief   returns balloon_type
**/
balloon_type_t get_balloon_type(void);

/**
 * @brief   sets capture_image
**/
void set_capture_image(bool capture);

/**
 * @brief   Starts the camera, image capture and image processing threads.
 * @return  none
**/
void process_image_start(void);

#endif /* PROCESS_IMAGE_H */