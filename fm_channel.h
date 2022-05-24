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
	cc_bool key_on;
} FM_Channel_State;

typedef struct FM_Channel
{
	const FM_Channel_Constant *constant;
	FM_Channel_State *state;
} FM_Channel;

void FM_Channel_Constant_Initialise(FM_Channel_Constant *constant);
void FM_Channel_State_Initialise(FM_Channel_State *state);

void FM_Channel_SetFrequency(const FM_Channel *channel, unsigned int f_number_and_block);
void FM_Channel_SetFeedbackAndAlgorithm(const FM_Channel *channel, unsigned int feedback, unsigned int algorithm);
void FM_Channel_SetDetuneAndMultiplier(const FM_Channel *channel, unsigned int operator_index, unsigned int detune, unsigned int multiplier);
void FM_Channel_SetTotalLevel(const FM_Channel *channel, unsigned int operator_index, unsigned int total_level);

int FM_Channel_GetSample(const FM_Channel *channel);

#endif /* FM_CHANNEL_H */
