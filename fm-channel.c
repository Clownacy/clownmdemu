#include "fm-channel.h"

#include <assert.h>

#include "clowncommon/clowncommon.h"

void FM_Channel_Constant_Initialise(FM_Channel_Constant* const constant)
{
	FM_Operator_Constant_Initialise(&constant->operators);
}

void FM_Channel_State_Initialise(FM_Channel_State* const state)
{
	cc_u16f i;

	for (i = 0; i < CC_COUNT_OF(state->operators); ++i)
		FM_Operator_State_Initialise(&state->operators[i]);

	state->feedback_divisor = 1 << (9 - 0);
	state->algorithm = 0;

	for (i = 0; i < CC_COUNT_OF(state->operator_1_previous_samples); ++i)
		state->operator_1_previous_samples[i] = 0;
}

void FM_Channel_Parameters_Initialise(FM_Channel* const channel, const FM_Channel_Constant* const constant, FM_Channel_State* const state)
{
	cc_u16f i;

	channel->constant = constant;
	channel->state = state;

	for (i = 0; i < CC_COUNT_OF(channel->operators); ++i)
	{
		channel->operators[i].constant = &constant->operators;
		channel->operators[i].state = &state->operators[i];
	}
}

void FM_Channel_SetFrequency(const FM_Channel* const channel, const cc_u16f f_number_and_block)
{
	cc_u16f i;

	for (i = 0; i < CC_COUNT_OF(channel->state->operators); ++i)
		FM_Operator_SetFrequency(&channel->operators[i], f_number_and_block);
}

void FM_Channel_SetFrequencies(const FM_Channel* const channel, const cc_u16l* const f_number_and_block)
{
	cc_u16f i;

	for (i = 0; i < CC_COUNT_OF(channel->state->operators); ++i)
		FM_Operator_SetFrequency(&channel->operators[i], f_number_and_block[i]);
}

void FM_Channel_SetFeedbackAndAlgorithm(const FM_Channel* const channel, const cc_u16f feedback, const cc_u16f algorithm)
{
	channel->state->feedback_divisor = 1 << (9 - feedback);
	channel->state->algorithm = algorithm;
}

void FM_Channel_SetSSGEG(const FM_Channel* const channel, const cc_u8f ssgeg)
{
	cc_u16f i;

	for (i = 0; i < CC_COUNT_OF(channel->state->operators); ++i)
		FM_Operator_SetSSGEG(&channel->operators[i], ssgeg);
}

void FM_Channel_SetKeyOn(const FM_Channel* const channel, const cc_u16f operator_index, const cc_bool key_on)
{
	FM_Operator_SetKeyOn(&channel->operators[operator_index], key_on);
}

void FM_Channel_SetDetuneAndMultiplier(const FM_Channel* const channel, const cc_u16f operator_index, const cc_u16f detune, const cc_u16f multiplier)
{
	FM_Operator_SetDetuneAndMultiplier(&channel->operators[operator_index], detune, multiplier);
}

void FM_Channel_SetTotalLevel(const FM_Channel* const channel, const cc_u16f operator_index, const cc_u16f total_level)
{
	FM_Operator_SetTotalLevel(&channel->operators[operator_index], total_level);
}

void FM_Channel_SetKeyScaleAndAttackRate(const FM_Channel* const channel, const cc_u16f operator_index, const cc_u16f key_scale, const cc_u16f attack_rate)
{
	FM_Operator_SetKeyScaleAndAttackRate(&channel->operators[operator_index], key_scale, attack_rate);
}

void FM_Channel_SetDecayRate(const FM_Channel* const channel, const cc_u16f operator_index, const cc_u16f decay_rate)
{
	FM_Operator_SetDecayRate(&channel->operators[operator_index], decay_rate);
}

void FM_Channel_SetSustainRate(const FM_Channel* const channel, const cc_u16f operator_index, const cc_u16f sustain_rate)
{
	FM_Operator_SetSustainRate(&channel->operators[operator_index], sustain_rate);
}

void FM_Channel_SetSustainLevelAndReleaseRate(const FM_Channel* const channel, const cc_u16f operator_index, const cc_u16f sustain_level, const cc_u16f release_rate)
{
	FM_Operator_SetSustainLevelAndReleaseRate(&channel->operators[operator_index], sustain_level, release_rate);
}

cc_s16f FM_Channel_GetSample(const FM_Channel* const channel)
{
	const FM_Operator* const operator1 = &channel->operators[0];
	const FM_Operator* const operator2 = &channel->operators[2]; /* Yes, these really are swapped. */
	const FM_Operator* const operator3 = &channel->operators[1];
	const FM_Operator* const operator4 = &channel->operators[3];

	cc_s16f feedback_modulation;
	cc_s16f operator_1_sample;
	cc_s16f operator_2_sample;
	cc_s16f operator_3_sample;
	cc_s16f operator_4_sample;
	cc_s16f sample;

	/* Compute operator 1's self-feedback modulation. */
	if (channel->state->feedback_divisor == 1 << (9 - 0))
		feedback_modulation = 0;
	else
		feedback_modulation = (channel->state->operator_1_previous_samples[0] + channel->state->operator_1_previous_samples[1]) / channel->state->feedback_divisor;

	/* Feed the operators into each other to produce the final sample. */
	/* Note that the operators output a 14-bit sample, meaning that, if all four are summed, then the result is a 16-bit sample,
	   so there is no possibility of overflow. */
	switch (channel->state->algorithm)
	{
		default:
			/* Should not happen. */
			assert(0);
			/* Fallthrough */
		case 0:
			/* "Four serial connection mode". */
			operator_1_sample = FM_Operator_Process(operator1, feedback_modulation);
			operator_2_sample = FM_Operator_Process(operator2, operator_1_sample);
			operator_3_sample = FM_Operator_Process(operator3, operator_2_sample);
			operator_4_sample = FM_Operator_Process(operator4, operator_3_sample);

			sample = operator_4_sample;

			break;

		case 1:
			/* "Three double modulation serial connection mode". */
			operator_1_sample = FM_Operator_Process(operator1, feedback_modulation);
			operator_2_sample = FM_Operator_Process(operator2, 0);

			operator_3_sample = FM_Operator_Process(operator3, operator_1_sample + operator_2_sample);
			operator_4_sample = FM_Operator_Process(operator4, operator_3_sample);

			sample = operator_4_sample;

			break;

		case 2:
			/* "Double modulation mode (1)". */
			operator_1_sample = FM_Operator_Process(operator1, feedback_modulation);

			operator_2_sample = FM_Operator_Process(operator2, 0);
			operator_3_sample = FM_Operator_Process(operator3, operator_2_sample);

			operator_4_sample = FM_Operator_Process(operator4, operator_1_sample + operator_3_sample);

			sample = operator_4_sample;

			break;

		case 3:
			/* "Double modulation mode (2)". */
			operator_1_sample = FM_Operator_Process(operator1, feedback_modulation);
			operator_2_sample = FM_Operator_Process(operator2, operator_1_sample);

			operator_3_sample = FM_Operator_Process(operator3, 0);

			operator_4_sample = FM_Operator_Process(operator4, operator_2_sample + operator_3_sample);

			sample = operator_4_sample;

			break;

		case 4:
			/* "Two serial connection and two parallel modes". */
			operator_1_sample = FM_Operator_Process(operator1, feedback_modulation);
			operator_2_sample = FM_Operator_Process(operator2, operator_1_sample);

			operator_3_sample = FM_Operator_Process(operator3, 0);
			operator_4_sample = FM_Operator_Process(operator4, operator_3_sample);

			sample = operator_2_sample + operator_4_sample;

			break;

		case 5:
			/* "Common modulation 3 parallel mode". */
			operator_1_sample = FM_Operator_Process(operator1, feedback_modulation);

			operator_2_sample = FM_Operator_Process(operator2, operator_1_sample);
			operator_3_sample = FM_Operator_Process(operator3, operator_1_sample);
			operator_4_sample = FM_Operator_Process(operator4, operator_1_sample);

			sample = operator_2_sample + operator_3_sample + operator_4_sample;

			break;

		case 6:
			/* "Two serial connection + two sine mode". */
			operator_1_sample = FM_Operator_Process(operator1, feedback_modulation);
			operator_2_sample = FM_Operator_Process(operator2, operator_1_sample);

			operator_3_sample = FM_Operator_Process(operator3, 0);

			operator_4_sample = FM_Operator_Process(operator4, 0);

			sample = operator_2_sample + operator_3_sample + operator_4_sample;

			break;

		case 7:
			/* "Four parallel sine synthesis mode". */
			operator_1_sample = FM_Operator_Process(operator1, feedback_modulation);

			operator_2_sample = FM_Operator_Process(operator2, 0);

			operator_3_sample = FM_Operator_Process(operator3, 0);

			operator_4_sample = FM_Operator_Process(operator4, 0);

			sample = operator_1_sample + operator_2_sample + operator_3_sample + operator_4_sample;

			break;
	}

	/* Update the feedback values. */
	channel->state->operator_1_previous_samples[1] = channel->state->operator_1_previous_samples[0];
	channel->state->operator_1_previous_samples[0] = operator_1_sample;

	/* Clamp the sample to a signed 14-bit range. According to Nuked OPN2, real YM2612s actually reduce the sample to 9-bit and
	   clamp it to -0x100 and 0xFF instead, but I don't bother doing that here since having more bit depth makes for better audio. */
	return CC_CLAMP(-0x1FFF, 0x1FFF, sample);
}
