#ifndef PSG_H
#define PSG_H

#include <stddef.h>

#include "clowncommon.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PSG_NoiseType
{
	PSG_NOISE_TYPE_PERIODIC,
	PSG_NOISE_TYPE_WHITE
} PSG_NoiseType;

typedef struct PSG_ToneState
{
	/* Countdown until the phase changes */
	unsigned int countdown;
	/* Value to reset the countdown to when it expires */
	unsigned int countdown_master;
	/* The volume attenuation level of the channel */
	unsigned int attenuation;
	/* The current phase of the channel - 0 for high phase, and 1 for low phase */
	unsigned int output_bit;
} PSG_ToneState;

typedef struct PSG_NoiseState
{
	/* Countdown until the fake output bit alternates */
	unsigned int countdown;
	/* The volume attenuation level of the channel */
	unsigned int attenuation;
	/* The shift register is rotated when this bit goes from low to high */
	unsigned int fake_output_bit;
	/* The current phase of the channel - 0 for high phase, and 1 for low phase */
	unsigned int real_output_bit;
	/* Determines what the countdown is reset to when it expires:
	   0 - 0x10
	   1 - 0x20
	   2 - 0x40
	   3 - the same as the last tone channel */
	unsigned int frequency_mode;
	/* The type of noise output by the channel */
	PSG_NoiseType type;
	/* Rotating bitfield which is used to produce noise */
	unsigned int shift_register;
} PSG_NoiseState;

typedef struct PSG_LatchedCommand
{
	/* The channel that is currently latched:
	   0 = Tone channel 1
	   1 = Tone channel 2
	   2 = Tone channel 3
	   3 = Noise channel */
	unsigned int channel;
	/* Whether the latched command sets the volume attenuation or not */
	cc_bool is_volume_command;
} PSG_LatchedCommand;

typedef struct PSG_Persistent
{
	short volumes[0x10][2];
} PSG_Persistent;

typedef struct PSG_State
{
	/* The tone channels */
	PSG_ToneState tones[3];

	/* The noise channel*/
	PSG_NoiseState noise;

	/* Data relating to the latched command */
	PSG_LatchedCommand latched_command;
} PSG_State;

typedef struct PSG
{
	const PSG_Persistent *persistent;
	PSG_State *state;
} PSG;

void PSG_PersistentInitialise(PSG_Persistent *persistent);

/* Initialises the PSG_State struct with sane default values. */
/* All channels will be muted. */
void PSG_StateInitialise(PSG_State *state);

/* Processes a command. */
/* See https://www.smspower.org/Development/SN76489 for an explanation of the various commands. */
void PSG_DoCommand(const PSG *psg, unsigned int command);

/* Updates the PSG's internal state and outputs samples. */
/* The samples are mono and in signed 16-bit PCM format. */
void PSG_Update(const PSG *psg, short *sample_buffer, size_t total_samples);

#ifdef __cplusplus
}
#endif

#endif /* PSG_H */
