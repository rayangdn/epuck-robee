/**
 * @file	process_audio.c
 * @brief 	Handles the capture and the processing of audio data.
 * @note 	Inspired by TP5.
**/

//ChibiOS headers
#include <ch.h>
#include <hal.h>

//E-puck 2 headers
#include <audio/microphone.h>
#include <audio/audio_thread.h>
#include <arm_math.h>
#include <arm_const_structs.h>

//Project headers
#include "include/process_audio.h"


/*===========================================================================*/
/* File constants.                                                           */
/*===========================================================================*/

#define FFT_SIZE 1024

#define MIN_VALUE_THRESHOLD	10000 
#define DETECTION_THRESHOLD 5

#define MIN_FREQ		    10	//we don't analyze before this index to not use resources for nothing
#define FREQ_MOVE		    27	//415Hz
#define FREQ_COMMUNICATE	21	//330Hz
#define FREQ_STOP	        24	//370HZ
#define MAX_FREQ		    30	//we don't analyze after this index to not use resources for nothing

#define FREQ_MOVE_L		    	(FREQ_MOVE-1)
#define FREQ_MOVE_H		   	 	(FREQ_MOVE+1)
#define FREQ_COMMUNICATE_L		(FREQ_COMMUNICATE-1)
#define FREQ_COMMUNICATE_H		(FREQ_COMMUNICATE+1)
#define FREQ_STOP_L		        (FREQ_STOP-1)
#define FREQ_STOP_H		        (FREQ_STOP+1)


//notes used for the music playing
#define NOTE_AS3 233
#define NOTE_DS4 311
#define NOTE_F4  349
#define NOTE_DS4 311
#define NOTE_GS4 415
#define NOTE_AS4 466

#define NBR_NOTES 16

#define CHANGE_NOTE 16

/*===========================================================================*/
/* File local variables.                                                     */
/*===========================================================================*/

static mode_selected_t mode_activated = STOPPED;

//2 times FFT_SIZE because it contains complex numbers (real + imaginary)
static float micFront_cmplx_input[2 * FFT_SIZE];

//array containing the computed magnitude of the complex numbers
static float micFront_output[FFT_SIZE];

static uint16_t notes[NBR_NOTES] =  {NOTE_AS4, 0,  NOTE_AS4, 0, NOTE_GS4, 0, NOTE_AS4, 0, NOTE_F4, 0, NOTE_DS4, 0, NOTE_F4, 0, NOTE_AS3, 0};
static uint16_t tempos[NBR_NOTES] = {2*CHANGE_NOTE-7, CHANGE_NOTE-3, 2*CHANGE_NOTE, 2*CHANGE_NOTE, CHANGE_NOTE-5, CHANGE_NOTE/2, CHANGE_NOTE+5,
									 CHANGE_NOTE/2, 3*CHANGE_NOTE, 2*CHANGE_NOTE, CHANGE_NOTE-5, CHANGE_NOTE/2, CHANGE_NOTE+5, CHANGE_NOTE/2,
									 3*CHANGE_NOTE, 12*CHANGE_NOTE};

static uint16_t* note_pointer = &notes[0];

static bool playing_music = false;

/*===========================================================================*/
/* File local functions.                                                     */
/*===========================================================================*/

/**
 * @brief               Processes the audio data to perform actions.
 * @param[in] data      the audio data to process
**/
static void sound_remote(float* data){

	float max_norm = MIN_VALUE_THRESHOLD;
	int16_t max_norm_index = -1; 

	//we count the number of times we detect the same frequency to avoid detecting noise
	static uint8_t count_mode = 0;
	//if the music is playing, we don't want to change the mode until it is finised
	//to avoid detection errors
	if(playing_music)
	{
		return;
	}

	//search for the highest peak
	for(uint16_t i = MIN_FREQ ; i <= MAX_FREQ ; i++){
		if(data[i] > max_norm){
			max_norm = data[i];
			max_norm_index = i;
		}
	}

	//move
	if(max_norm_index >= FREQ_MOVE_L && max_norm_index <= FREQ_MOVE_H){
		++count_mode;
		if(count_mode >= DETECTION_THRESHOLD)
		{
			count_mode = 0;
			mode_activated = MOVING_TO_BALLOON;
			
		}
	}
	//communicate
	else if(max_norm_index >= FREQ_COMMUNICATE_L && max_norm_index <= FREQ_COMMUNICATE_H){
		++count_mode;
		if(count_mode >= DETECTION_THRESHOLD)
		{
			count_mode = 0;
			mode_activated = COMMUNICATING_WITH_PEERS;

		}
	}
	//stop
	else if(max_norm_index >= FREQ_STOP_L && max_norm_index <= FREQ_STOP_H){
		++count_mode;
		if(count_mode >= DETECTION_THRESHOLD)
		{
			count_mode = 0;
			mode_activated = STOPPED;
		}
	} else {
		count_mode = 0;
	}
}

/**
 * @brief               Computes the FFT of the audio data.
 * @param[in] size      the size of the FFT
 * @param[in] data      the audio data to process
 **/
static void doFFT_optimized(uint16_t size, float* complex_buffer){
	if(size == 1024)
		arm_cfft_f32(&arm_cfft_sR_f32_len1024, complex_buffer, 0, 1);
	
}

/**
 * @brief               	Processes the audio data to perform actions.
 * @param[in] data      	the audio data to process
 * @param[in] num_samples 	the number of samples to process
**/
static void process_audio_data(int16_t* data, uint16_t num_samples)
{
	static uint16_t nb_samples = 0;

	//loop to fill the buffers
	for(uint16_t i = 0 ; i < num_samples ; i+=4){
		//construct an array of complex numbers. Put 0 to the imaginary part
		micFront_cmplx_input[nb_samples] = (float)data[i + MIC_FRONT];
		nb_samples++;

		micFront_cmplx_input[nb_samples] = 0;
		nb_samples++;

		//stop when buffer is full
		if(nb_samples >= (2 * FFT_SIZE)){
			break;
		}
	}

	if(nb_samples >= (2 * FFT_SIZE)){
        //FFT procession
        //this FFT function stores the results in the input buffer given.
		doFFT_optimized(FFT_SIZE, micFront_cmplx_input);

		//magnitude processing
        //computes the magnitude of the complex numbers and stores them 
        //in a buffer of FFT_SIZE because it only contains real numbers.
		arm_cmplx_mag_f32(micFront_cmplx_input, micFront_output, FFT_SIZE);

		nb_samples = 0;
        //process the output to perform actions
		sound_remote(micFront_output);
	}
}

/*===========================================================================*/
/* File exported functions.                                                  */
/*===========================================================================*/

void play_music(bool end_music)
{
	static bool start_music = true;
    static uint16_t tempo_notes = 0;
    static uint8_t count_pointer = 0;

	if(start_music)
	{
		//starts DAC once
		dac_start();
		playing_music = true;
		start_music = false;
	}
	if(!end_music)
	{
		++tempo_notes;
		if(count_pointer == 0)
		{
			dac_play(*note_pointer);
		}
		//play the notes with the right tempo
		if(tempo_notes % tempos[count_pointer] == 0)
		{
			++note_pointer;
			dac_play(*note_pointer);
			++count_pointer;
			tempo_notes = 0;
		}
		if (count_pointer % NBR_NOTES == 0)
		{
			count_pointer = 0;
			note_pointer = &notes[0];
			
		}
	} else {
		note_pointer = &notes[0];
		tempo_notes = 0;
		count_pointer = 0;
		start_music = true;
		playing_music = false;
		dac_stop();
	}
}

mode_selected_t get_mode(void)
{
	return mode_activated;
}

void set_mode(mode_selected_t mode)
{
	mode_activated = mode;
}

void process_audio_start(void)
{
    //starts the microphones processing thread.
    //it calls the callback given in parameter when samples are ready
    mic_start(&process_audio_data);
}