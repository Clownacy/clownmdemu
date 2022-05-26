#ifndef FM_ENVELOPE_H
#define FM_ENVELOPE_H

#include "clowncommon.h"

typedef enum FM_Envelope_Mode
{
	FM_ENVELOPE_MODE_ATTACK  = 0,
	FM_ENVELOPE_MODE_DECAY   = 1,
	FM_ENVELOPE_MODE_SUSTAIN = 2,
	FM_ENVELOPE_MODE_RELEASE = 3
} FM_Envelope_Mode;

typedef struct FM_Envelope_State
{
	/* TODO: Make these two global. */
	unsigned int countdown;
	unsigned int cycle_counter;

	unsigned int delta_index;
	unsigned int current_attenuation;

	unsigned int total_level;
	unsigned int sustain_level;
	unsigned int key_scale;

	unsigned int rates[4];
	FM_Envelope_Mode current_mode;

	cc_bool key_on;
} FM_Envelope_State;

void FM_Envelope_State_Initialise(FM_Envelope_State *state);

void FM_Envelope_SetKeyOn(FM_Envelope_State *envelope, cc_bool key_on, unsigned int key_code);
void FM_Envelope_SetTotalLevel(FM_Envelope_State *envelope, unsigned int total_level);
void FM_Envelope_SetKeyScaleAndAttackRate(FM_Envelope_State *envelope, unsigned int key_scale, unsigned int attack_rate);
void FM_Envelope_DecayRate(FM_Envelope_State *envelope, unsigned int decay_rate);
void FM_Envelope_SetSustainRate(FM_Envelope_State *envelope, unsigned int sustain_rate);
void FM_Envelope_SetSustainLevelAndReleaseRate(FM_Envelope_State *envelope, unsigned int sustain_level, unsigned int release_rate);

unsigned int FM_Envelope_Update(FM_Envelope_State *envelope, unsigned int key_code);

#endif /* FM_ENVELOPE_H */
