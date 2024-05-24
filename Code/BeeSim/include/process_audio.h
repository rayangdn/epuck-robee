/**
 * @file	process_audio.h
 * @brief	Exported functions and constants related to
 * 			audio processing and capture.
**/

#ifndef PROCESS_AUDIO_H
#define PROCESS_AUDIO_H

/*===========================================================================*/
/* File data structures and types.                                           */
/*===========================================================================*/

//attribute packed to use one byte only
typedef enum __attribute__((__packed__)) mode_selected_t
{
    STOPPED,
    MOVING_TO_BALLOON,
    COMMUNICATING_WITH_PEERS
} mode_selected_t;

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

/**
 * @brief                   Play a predefined music.
 * @param[in]   end_music   the condition to stop the music
 * @return                  none
**/
void play_music(bool end_music);

/**
 * @brief   Returns mode_activated.
**/
mode_selected_t get_mode(void);

/**
 * @brief Set mode_activated.
**/
void set_mode(mode_selected_t mode);

/**
 * @brief   Starts the process audio thread.
 * @return  none
**/
void process_audio_start(void);

#endif /* PROCESS_AUDIO_H */