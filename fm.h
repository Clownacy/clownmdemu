#ifndef FM_H
#define FM_H

#include <stddef.h>

#include "clowncommon.h"

#include "fm_channel.h"

/* 8 is chosen because there are 6 FM channels (of which the DAC can replace one) as well as the PSG.
   The PSG with all of its channels at maximum volume reaches the volume of a single FM channel at maximum.
   Technically, this means that 7 is a more appropriate number than 8. However, dividing by 8 is simpler
   than dividing by 7, so that was opted for instead. */
#define FM_VOLUME_DIVIDER 8

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
