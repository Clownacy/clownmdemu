#ifndef PCM_H
#define PCM_H

#include <stddef.h>

#include "clowncommon/clowncommon.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PCM_ChannelState
{
	cc_bool disabled;
	cc_u8l volume;
	cc_u8l panning;
	cc_u16l frequency;
	cc_u16l loop_address;
	cc_u8l start_address;
	cc_u32l address;
} PCM_ChannelState;

typedef struct PCM_State
{
	PCM_ChannelState channels[8];
	cc_u8l wave_ram[0x10000];
	cc_bool sounding;
	cc_u8l current_wave_bank;
	cc_u8l current_channel;
} PCM_State;

typedef struct PCM
{
	PCM_State *state;
} PCM;

void PCM_State_Initialise(PCM_State *state);
void PCM_WriteRegister(const PCM *pcm, cc_u16f reg, cc_u8f value);
cc_u8f PCM_ReadRegister(const PCM *pcm, cc_u8f reg);
void PCM_WriteWaveRAM(const PCM *pcm, cc_u16f address, cc_u8f value);
void PCM_Update(const PCM *pcm, cc_s16l *sample_buffer, size_t total_samples);

#ifdef __cplusplus
}
#endif

#endif /* PCM_H */
