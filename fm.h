#ifndef FM_H
#define FM_H

#include <stddef.h>

#define LENGTH_OF_SINE_WAVE_LOOKUP_TABLE 0x1000

typedef struct FM_Channel
{
	unsigned long sine_wave_position;
	unsigned long sine_wave_step;
	unsigned int attenuation;
} FM_Channel;


typedef struct FM_Constant
{
	short sine_waves[0x80][LENGTH_OF_SINE_WAVE_LOOKUP_TABLE];
} FM_Constant;

typedef struct FM_State
{
	FM_Channel channels[6];
	unsigned int port;
	unsigned int address;
	unsigned int cached_data;
} FM_State;

typedef struct FM
{
	const FM_Constant *constant;
	FM_State *state;
} FM;

void FM_Constant_Initialise(FM_Constant *constant);
void FM_State_Initialise(FM_State *state);

void FM_DoAddress(const FM *fm, unsigned int port, unsigned int address);
void FM_DoData(const FM *fm, unsigned int data);

/* Updates the FM's internal state and outputs samples. */
/* The samples are stereo and in signed 16-bit PCM format. */
void FM_Update(const FM *fm, short *sample_buffer, size_t total_frames);

#endif /* FM_H */
