#ifndef FM_OPERATOR_H
#define FM_OPERATOR_H

#include "fm_phase.h"

typedef struct FM_Operator_Constant
{
	unsigned short logarithmic_attenuation_sine_table[0x100];
	unsigned short power_table[0x100];
} FM_Operator_Constant;

typedef struct FM_Operator_State
{
	FM_Phase_State phase;
	unsigned int total_level;
} FM_Operator_State;

typedef struct FM_Operator
{
	const FM_Operator_Constant *constant;
	FM_Operator_State *state;
} FM_Operator;

void FM_Operator_Constant_Initialise(FM_Operator_Constant *constant);
void FM_Operator_State_Initialise(FM_Operator_State *state);

void FM_Operator_SetFrequency(const FM_Operator *fm_operator, unsigned int f_number_and_block);
void FM_Operator_SetDetuneAndMultiplier(const FM_Operator *fm_operator, unsigned int detune, unsigned int multiplier);
void FM_Operator_SetTotalLevel(const FM_Operator *fm_operator, unsigned int total_level);

int FM_Operator_Process(const FM_Operator *fm_operator, int phase_modulation);

#endif /* FM_OPERATOR_H */
