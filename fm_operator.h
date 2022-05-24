#ifndef FM_OPERATOR_H
#define FM_OPERATOR_H

#include "clowncommon.h"

#include "fm_phase.h"
#include "fm_envelope.h"

typedef struct FM_Operator_Constant
{
	unsigned short logarithmic_attenuation_sine_table[0x100];
	unsigned short power_table[0x100];
} FM_Operator_Constant;

typedef struct FM_Operator_State
{
	FM_Phase_State phase;
	FM_Envelope_State envelope;
} FM_Operator_State;

typedef struct FM_Operator
{
	const FM_Operator_Constant *constant;
	FM_Operator_State *state;
} FM_Operator;

void FM_Operator_Constant_Initialise(FM_Operator_Constant *constant);
void FM_Operator_State_Initialise(FM_Operator_State *state);

void FM_Operator_SetFrequency(const FM_Operator *fm_operator, unsigned int f_number_and_block);
void FM_Operator_SetKeyOn(const FM_Operator *fm_operator, cc_bool key_on);
void FM_Operator_SetDetuneAndMultiplier(const FM_Operator *fm_operator, unsigned int detune, unsigned int multiplier);
void FM_Operator_SetTotalLevel(const FM_Operator *fm_operator, unsigned int total_level);
void FM_Operator_SetKeyScaleAndAttackRate(const FM_Operator *fm_operator, unsigned int key_scale, unsigned int attack_rate);
void FM_Operator_DecayRate(const FM_Operator *fm_operator, unsigned int decay_rate);
void FM_Operator_SetSustainRate(const FM_Operator *fm_operator, unsigned int sustain_rate);
void FM_Operator_SetSustainLevelAndReleaseRate(const FM_Operator *fm_operator, unsigned int sustain_level, unsigned int release_rate);

int FM_Operator_Process(const FM_Operator *fm_operator, int phase_modulation);

#endif /* FM_OPERATOR_H */
