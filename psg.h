#ifndef PSG_H
#define PSG_H

#include <stddef.h>

#include "clowncommon.h"

typedef struct PSG_ToneState
{
	unsigned int countdown;
	unsigned int countdown_master;
	unsigned int attenuation;
	unsigned int output_bit;
} PSG_ToneState;

typedef struct PSG_State
{
	PSG_ToneState tones[3];
	struct
	{
		unsigned int countdown;
		unsigned int attenuation;
		unsigned int fake_output_bit;
		unsigned int real_output_bit;
		unsigned int frequency_mode;
		cc_bool periodic_mode;
		unsigned int shift_register;
	} noise;
	struct
	{
		unsigned int channel;
		unsigned int mode;
	} latched_command;
	int volumes[0x10][2];
} PSG_State;

void PSG_Init(PSG_State *state);
void PSG_Update(PSG_State *state, short *sample_buffer, size_t total_samples);
void PSG_DoCommand(PSG_State *state, unsigned int command);

#endif /* PSG_H */