#ifndef FM_CHANNEL_H
#define FM_CHANNEL_H

#include "clowncommon.h"

#include "fm_operator.h"

typedef struct FM_Channel_Constant
{
	FM_Operator_Constant operators;
} FM_Channel_Constant;

typedef struct FM_Channel_State
{
	FM_Operator_State operators[4];
	unsigned int feedback;
	unsigned int algorithm;
	unsigned int operator_1_previous_samples[2];
} FM_Channel_State;

typedef struct FM_Channel
{
	const FM_Channel_Constant *constant;
	FM_Channel_State *state;
} FM_Channel;

void FM_Channel_Constant_Initialise(FM_Channel_Constant *constant);
void FM_Channel_State_Initialise(FM_Channel_State *state);

/* Per-channel. */
void FM_Channel_SetFrequency(const FM_Channel *channel, unsigned int f_number_and_block);
void FM_Channel_SetFeedbackAndAlgorithm(const FM_Channel *channel, unsigned int feedback, unsigned int algorithm);

/* Per-operator. */
void FM_Channel_SetKeyOn(const FM_Channel *channel, unsigned int operator_index, cc_bool key_on);
void FM_Channel_SetDetuneAndMultiplier(const FM_Channel *channel, unsigned int operator_index, unsigned int detune, unsigned int multiplier);
void FM_Channel_SetTotalLevel(const FM_Channel *channel, unsigned int operator_index, unsigned int total_level);
void FM_Channel_SetKeyScaleAndAttackRate(const FM_Channel *channel, unsigned int operator_index, unsigned int key_scale, unsigned int attack_rate);
void FM_Channel_DecayRate(const FM_Channel *channel, unsigned int operator_index, unsigned int decay_rate);
void FM_Channel_SetSustainRate(const FM_Channel *channel, unsigned int operator_index, unsigned int sustain_rate);
void FM_Channel_SetSustainLevelAndReleaseRate(const FM_Channel *channel, unsigned int operator_index, unsigned int sustain_level, unsigned int release_rate);

int FM_Channel_GetSample(const FM_Channel *channel);

#endif /* FM_CHANNEL_H */
