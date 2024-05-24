/**
 * @file    controller.c
 * @brief   Handles the control of the robot.
**/

//C headers
#include <stdlib.h>

//ChibiOS headers
#include <ch.h>
#include <hal.h>

//E-puck 2 headers
#include <motors.h>
#include <leds.h>

//Project headers
#include "include/controller.h"
#include "include/process_image.h"
#include "include/process_audio.h"
#include "include/TOF_sensor.h"

/*===========================================================================*/
/* File constants.                                                           */
/*===========================================================================*/

//speed constants
#define NORMAL_SPEED 150
#define ROTATION_THRESHOLD 10
#define ROTATION_COEFF 2

//detection constants
#define GOAL_DISTANCE 50

//P controller constant
#define KP 2

//attack constants
#define ATTACK_ENEMY 40
#define ROTATE_180_DEGREES 138

//pollinate constants
#define MOVE_FORWARD 180
#define GIGGLE_FLOWER 17 

//communicate constants
#define ROTATE_360_DEGREES 352

/*===========================================================================*/
/* File local variables.                                                     */
/*===========================================================================*/

//in case of a non expected mode change, we reset every static variablle to default 
//to avoid any unwanted behavior
static bool reset_variable = false;

/*===========================================================================*/
/* Local functions.                                                          */
/*===========================================================================*/

/**
 * @brief           Sets the LEDs of the robot.
 * @param   red     the red value of the LED
 * @param   green   the green value of the LED
 * @param   blue    the blue value of the LED
 * @return          none
**/
static void set_leds(uint8_t red, uint8_t green, uint8_t blue)
{
    for (int i = LED2; i <= LED8; i++)
    {
        set_rgb_led(i, red, green, blue);
    }
}

/**
 * @brief   Researches a balloon by turning on itself
 * @return  none
**/
static void research_balloon(void)
{
    right_motor_set_speed(NORMAL_SPEED);
    left_motor_set_speed(-NORMAL_SPEED);
}

static bool approach_balloon(uint16_t balloon_position)
{
    uint16_t error = 0;
    uint16_t speed = 0;
    int16_t speed_correction = balloon_position - IMAGE_BUFFER_SIZE/2;

    //get the distance to the balloon
    error = get_TOF_value();

    //P controller implementation
    speed = KP * error;

    //if the speed is too low, we set it to the normal speed
    if(speed < NORMAL_SPEED)
    {
        speed = NORMAL_SPEED;
    }
   
    //threshold to avoid small corrections due to noise of the camera
    if(abs(speed_correction) < ROTATION_THRESHOLD)
    {
        speed_correction = 0;
    }
    right_motor_set_speed(speed - ROTATION_COEFF*speed_correction);
    left_motor_set_speed(speed + ROTATION_COEFF*speed_correction);

    if (error < GOAL_DISTANCE)
    {
        right_motor_set_speed(0);
        left_motor_set_speed(0);
        return true;
    }
    return false;
}

/**
 * @brief   Attacks the enemy by turning on itselfs and moving backward.
 * @return  true if the robot is still attacking, false otherwise
**/
static bool attack_enemy(void)
{
    static uint8_t count_rotation = 0;
    static uint8_t count_move_backward = 0;

    if(reset_variable)
    {
        count_rotation = 0;
        count_move_backward = 0;
        return 0;
    }
    ++count_rotation;
    if(count_rotation <= ROTATE_180_DEGREES) {
        right_motor_set_speed(3*NORMAL_SPEED);
        left_motor_set_speed(-3*NORMAL_SPEED);
    } else {
        ++count_move_backward;
        right_motor_set_speed(-10*NORMAL_SPEED);
        left_motor_set_speed(-10*NORMAL_SPEED);
        if(count_move_backward >= ATTACK_ENEMY) {
            count_rotation = 0;
            count_move_backward = 0;
            right_motor_set_speed(0);
            left_motor_set_speed(0);
            //allows the camera to capture image again
            set_capture_image(true);
            return false;
        }
    }
    return true;
}

/**
 * @brief   Makes the robot pollinate with a flower.
 * @return  true if the robot is still performing the action, false otherwise
**/
static bool pollinate_flower(void)
{
    static uint8_t count_move_forward = 0;
    static uint8_t count_rotation = 0;
    static int16_t speed = NORMAL_SPEED;

    if(reset_variable)
    {
        count_move_forward = 0;
        count_rotation = 0;
        return 0;
    }
    
    if(count_move_forward <= MOVE_FORWARD) {
        ++count_move_forward;
        right_motor_set_speed(speed);
        left_motor_set_speed(speed);
    } else {
        ++count_rotation;
        if(count_rotation % GIGGLE_FLOWER == 0) {
            speed = -speed;
            right_motor_set_speed(speed);
            left_motor_set_speed(-speed);
        }
        if(count_rotation >= 10 * GIGGLE_FLOWER) {
            count_rotation = 0;
            count_move_forward = 0;
            right_motor_set_speed(0);
            left_motor_set_speed(0);
            //allows the camera to capture image again
            set_capture_image(true);
            return false;
        }
    }
    return true;
}

/**
 * @brief   Moves the robot to the balloon.
 * @return  none
**/
static void move_to_balloon(void)
{
    //initial state
    static action_type_t action_type = SEARCHING;

    //get the infos from the image processing
    uint16_t balloon_position = get_balloon_position();
    balloon_type_t balloon_type = get_balloon_type();

    if(reset_variable)
    {
        action_type = SEARCHING;
        return;
    }
   
    if(balloon_type == NONE)
    {
        set_capture_image(true);
        action_type = SEARCHING;
    } 
    switch (action_type)
    {
        case SEARCHING:
            //yellow
            set_leds(255, 255, 0);
             //turns on itself
            research_balloon();
            action_type = APPROACHING;
            break;
        case APPROACHING:
            //yellow
            set_leds(255, 255, 0);
            //if the robot is close enough to the balloon
            if(approach_balloon(balloon_position)) {
                
                if (balloon_type == FLOWER)
                {
                    //blue
                    set_leds(0, 0, 255);
                    action_type = POLLINATING;
                } else {
                    //red
                    set_leds(255,  0, 0);
                    action_type = ATTACKING;
                }
            }
            break;
        case POLLINATING:
            
            if (!pollinate_flower())
            {
                action_type = SEARCHING;
            }
            break;
        case ATTACKING:
            if(!attack_enemy())
            {
                action_type = SEARCHING;
            }
            break;
        default:
            //no color
            set_leds(0, 0, 0);
            left_motor_set_speed(0);
            right_motor_set_speed(0);
            break;
    }
}

/**
 * @brief   Communicates with other bees by doing complete turn and playing music.
 * @return  none
**/
static void communicate_with_peers(void)
{
    static uint16_t count_rotation = 0;
    static int16_t speed = 3*NORMAL_SPEED;

    if(reset_variable)
    {
        count_rotation = 0;
        play_music(true);
        return;
    }

    if(count_rotation <= 4*ROTATE_360_DEGREES)
    {
        play_music(false);
        ++count_rotation;
        if(count_rotation % ROTATE_360_DEGREES == 0)
        {
            speed = -speed;
        }
        left_motor_set_speed(-speed);
        right_motor_set_speed(speed);
        
    } else {
        play_music(true);
        left_motor_set_speed(0);
        right_motor_set_speed(0);
        count_rotation = 0;
        set_mode(MOVING_TO_BALLOON);
    }
}

static void reset_all(void)
{
    reset_variable = true;

    //resets every static variables in every function to default values
    communicate_with_peers();
    move_to_balloon();
    attack_enemy();
    pollinate_flower();
    set_capture_image(true);
    
    reset_variable = false;
}

/*===========================================================================*/
/* File threads.                                                             */
/*===========================================================================*/

static THD_WORKING_AREA(waController, 512);
static THD_FUNCTION(Controller, arg) 
{

     chRegSetThreadName(__FUNCTION__);
    (void)arg;

    systime_t time;

    mode_selected_t last_mode = get_mode();
    mode_selected_t current_mode = get_mode();
    
    while(1){
        time = chVTGetSystemTime();
        current_mode = get_mode();
        if(last_mode != current_mode)
        {
            last_mode = current_mode;
            reset_all();
        }
        switch (current_mode)
        {
            case MOVING_TO_BALLOON:
                move_to_balloon();
                break;
            case COMMUNICATING_WITH_PEERS:
                //magenta
                set_leds(255, 0, 255);
                communicate_with_peers();
                break;
            case STOPPED:
            default:
                //no color
                set_leds(0, 0, 0);
                right_motor_set_speed(0);
                left_motor_set_speed(0);
                break;
        }
        //100Hz precisly
        chThdSleepUntilWindowed(time, time + MS2ST(10));
    }
}

/*===========================================================================*/
/* File exported functions.                                                  */
/*===========================================================================*/

void controller_start(void) {
    //initializes the motors
    motors_init();
    //starts the controller thread
	chThdCreateStatic(waController, sizeof(waController), NORMALPRIO, Controller, NULL);
}