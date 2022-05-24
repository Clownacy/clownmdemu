#ifndef FM_H
#define FM_H

#include <stddef.h>

#include "clowncommon.h"

#include "fm_channel.h"

typedef struct FM_ChannelMetadata
{
	FM_Channel_State state;
	unsigned int cached_upper_frequency_bits;
	cc_bool pan_left;
	cc_bool pan_right;
} FM_ChannelMetadata;

typedef struct FM_Constant
{
	FM_Channel_Constant channels;
} FM_Constant;

typedef struct FM_State
{
	FM_ChannelMetadata channels[6];
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
