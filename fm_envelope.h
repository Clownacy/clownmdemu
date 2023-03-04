#ifndef FM_ENVELOPE_H
#define FM_ENVELOPE_H

#include "clowncommon/clowncommon.h"

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
	cc_u16l countdown;
	cc_u16l cycle_counter;

	cc_u16l delta_index;
	cc_u16l current_attenuation;

	cc_u16l total_level;
	cc_u16l sustain_level;
	cc_u16l key_scale;

	cc_u16l rates[4];
	FM_Envelope_Mode current_mode;

	cc_bool key_on;
} FM_Envelope_State;

void FM_Envelope_State_Initialise(FM_Envelope_State *state);

cc_bool FM_Envelope_SetKeyOn(FM_Envelope_State *envelope, cc_bool key_on, cc_u16f key_code);
void FM_Envelope_SetTotalLevel(FM_Envelope_State *envelope, cc_u16f total_level);
void FM_Envelope_SetKeyScaleAndAttackRate(FM_Envelope_State *envelope, cc_u16f key_scale, cc_u16f attack_rate);
void FM_Envelope_DecayRate(FM_Envelope_State *envelope, cc_u16f decay_rate);
void FM_Envelope_SetSustainRate(FM_Envelope_State *envelope, cc_u16f sustain_rate);
void FM_Envelope_SetSustainLevelAndReleaseRate(FM_Envelope_State *envelope, cc_u16f sustain_level, cc_u16f release_rate);

cc_u16f FM_Envelope_Update(FM_Envelope_State *envelope, cc_u16f key_code);

#endif /* FM_ENVELOPE_H */
