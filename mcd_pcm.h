#ifndef MCD_PCM_H
#define MCD_PCM_H

#include <stddef.h>

#include "clowncommon/clowncommon.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MCD_PCM_ChannelState
{
	cc_bool disabled;
	cc_u8l volume;
	cc_u8l panning;
	cc_u16l frequency;
	cc_u16l loop_address;
	cc_u8l start_address;
	cc_u32l address;
} MCD_PCM_ChannelState;

typedef struct MCD_PCM_State
{
	MCD_PCM_ChannelState channels[8];
	cc_u8l wave_ram[0x10000];
	cc_bool sounding;
	cc_u8l current_wave_bank;
	cc_u8l current_channel;
} MCD_PCM_State;

typedef struct MCD_PCM
{
	MCD_PCM_State *state;
} MCD_PCM;

void MCD_PCM_State_Initialise(MCD_PCM_State *state);
void MCD_PCM_WriteRegister(const MCD_PCM *pcm, cc_u16f reg, cc_u8f value);
cc_u8f MCD_PCM_ReadRegister(const MCD_PCM *pcm, cc_u8f reg);
void MCD_PCM_WriteWaveRAM(const MCD_PCM *pcm, cc_u16f address, cc_u8f value);
void MCD_PCM_Update(const MCD_PCM *pcm, cc_s16l *sample_buffer, size_t total_samples);

#ifdef __cplusplus
}
#endif

#endif /* MCD_PCM_H */