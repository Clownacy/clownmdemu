#ifndef FM_H
#define FM_H

#include <stddef.h>

#include "clowncommon.h"

#include "fm_operator.h"
#include "fm_phase.h"

#define LENGTH_OF_SINE_WAVE_LOOKUP_TABLE 0x400

typedef struct FM_Operator
{
	FM_Phase phase;
	unsigned int attenuation;
	cc_bool is_slot;
} FM_Operator;

typedef struct FM_Channel
{
	FM_Operator operators[4];
	unsigned int cached_upper_frequency_bits;
	cc_bool key_on;
	cc_bool pan_left;
	cc_bool pan_right;
} FM_Channel;

typedef struct FM_Constant
{
	FM_Operator_Constant operators;
} FM_Constant;

typedef struct FM_State
{
	FM_Channel channels[6];
	unsigned int port;
	unsigned int address;
	int dac_sample;
	cc_bool dac_enabled;
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
