/* Based on Nemesis's notes: http://gendev.spritesmind.net/forum/viewtopic.php?p=6114#p6114 */

#include "fm_operator.h"

#include <math.h>

#include "clowncommon.h"

static unsigned int InversePow2(const FM_Operator_Constant *constant, unsigned int value)
{
	/* TODO: Maybe replace this whole thing with a single lookup table? */

	/* The attenuation is in 5.8 fixed point format. */
	const unsigned int whole = value >> 8;
	const unsigned int fraction = value & 0xFF;

	return (constant->power_table[fraction] << 2) >> whole;
}

void FM_Operator_Constant_Initialise(FM_Operator_Constant *constant)
{
	const unsigned int sine_table_length = CC_COUNT_OF(constant->logarithmic_attenuation_sine_table);
	const unsigned int pow_table_length = CC_COUNT_OF(constant->power_table);
	const double log2 = log(2.0);

	unsigned int i;

	/* Generate sine wave lookup table. */
	for (i = 0; i < sine_table_length; ++i)
	{
		/* "Calculate the normalized phase value for the input into the sine table.Note
		    that this is calculated as a normalized result from 0.0-1.0 where 0 is not
		    reached, because the phase is calculated as if it was a 9-bit index with the
		    LSB fixed to 1. This was done so that the sine table would be more accurate
		    when it was "mirrored" to create the negative oscillation of the wave. It's
		    also convenient we don't have to worry about a phase of 0, because 0 is an
		    invalid input for a log function, which we need to use below." */
		const double phase_normalised = (double)((i << 1) + 1) / (double)(sine_table_length << 1);

		/* "Calculate the pure sine value for the input.Note that we only build a sine
		    table for a quarter of the full oscillation (0-PI/2), since the upper two bits
		    of the full phase are extracted by the external circuit." */
		const double sin_result_normalized = sin(phase_normalised * (CC_PI / 2.0));

		/* "Convert the sine result from a linear representation of volume, to a
		    logarithmic representation of attenuation. The YM2612 stores values in the sine
		    table in this form because logarithms simplify multiplication down to addition,
		    and this allowed them to attenuate the sine result by the envelope generator
		    output simply by adding the two numbers together." */
		const double sin_result_as_attenuation = -log(sin_result_normalized) / log2;
		/* "The division by log(2) is required because the log function is base 10, but the
		    YM2612 uses a base 2 logarithmic value. Dividing the base 10 log result by
		    log10(2) will convert the result to a base 2 logarithmic value, which can then
		    be converted back to a linear value by a pow2 function. In other words:
		    2^(log10(x)/log10(2)) = 2^log2(x) = x
		    If there was a native log2() function provided we could use that instead." */

		/* "Convert the attenuation value to a rounded 12-bit result in 4.8 fixed point
		    format." */
		const unsigned int sinResult = (unsigned int)((sin_result_as_attenuation * 256.0) + 0.5);

		/* "Write the result to the table." */
		constant->logarithmic_attenuation_sine_table[i] = sinResult;
	}

	/* Generate power lookup table. */
	for (i = 0; i < pow_table_length; ++i)
	{
		/* "Normalize the current index to the range 0.0 - 1.0.Note that in this case, 0.0
		    is a value which is never actually reached, since we start from i+1. They only
		    did this to keep the result to an 11-bit output. It probably would have been
		    better to simply subtract 1 from every final number and have 1.0 as the input
		    limit instead when building the table, so an input of 0 would output 0x7FF,
		    but they didn't." */
		const double entry_normalised = (double)(i + 1) / (double)pow_table_length;

		/* "Calculate 2 ^ -entryNormalized." */
		const double result_normalised = pow(2.0, -entry_normalised);

		/* "Convert the normalized result to an 11-bit rounded result." */
		const unsigned int result = (unsigned int)((result_normalised * 2048.0) + 0.5);

		/* "Write the result to the table." */
		constant->power_table[i] = result;
	}
}

void FM_Operator_State_Initialise(FM_Operator_State *state)
{
	FM_Phase_State_Initialise(&state->phase);
	FM_Envelope_State_Initialise(&state->envelope);
}

void FM_Operator_SetFrequency(const FM_Operator *fm_operator, unsigned int f_number_and_block)
{
	FM_Phase_SetFrequency(&fm_operator->state->phase, f_number_and_block);
}

void FM_Operator_SetKeyOn(const FM_Operator *fm_operator, cc_bool key_on)
{
	/* If we switch from key-off to key-on, then reset the Phase Generator. */
	if (FM_Envelope_SetKeyOn(&fm_operator->state->envelope, key_on, FM_Phase_GetKeyCode(&fm_operator->state->phase)))
		FM_Phase_Reset(&fm_operator->state->phase);
}

void FM_Operator_SetDetuneAndMultiplier(const FM_Operator *fm_operator, unsigned int detune, unsigned int multiplier)
{
	FM_Phase_SetDetuneAndMultiplier(&fm_operator->state->phase, detune, multiplier);
}

void FM_Operator_SetTotalLevel(const FM_Operator *fm_operator, unsigned int total_level)
{
	FM_Envelope_SetTotalLevel(&fm_operator->state->envelope, total_level);
}

void FM_Operator_SetKeyScaleAndAttackRate(const FM_Operator *fm_operator, unsigned int key_scale, unsigned int attack_rate)
{
	FM_Envelope_SetKeyScaleAndAttackRate(&fm_operator->state->envelope, key_scale, attack_rate);
}

void FM_Operator_DecayRate(const FM_Operator *fm_operator, unsigned int decay_rate)
{
	FM_Envelope_DecayRate(&fm_operator->state->envelope, decay_rate);
}

void FM_Operator_SetSustainRate(const FM_Operator *fm_operator, unsigned int sustain_rate)
{
	FM_Envelope_SetSustainRate(&fm_operator->state->envelope, sustain_rate);
}

void FM_Operator_SetSustainLevelAndReleaseRate(const FM_Operator *fm_operator, unsigned int sustain_level, unsigned int release_rate)
{
	FM_Envelope_SetSustainLevelAndReleaseRate(&fm_operator->state->envelope, sustain_level, release_rate);
}

int FM_Operator_Process(const FM_Operator *fm_operator, int phase_modulation)
{
	/* Update and obtain phase and make it 10-bit (the upper bits are discarded later). */
	const unsigned int phase = FM_Phase_Increment(&fm_operator->state->phase) >> 10;

	/* Update and obtain attenuation (10-bit). */
	const unsigned int attenuation = FM_Envelope_Update(&fm_operator->state->envelope, FM_Phase_GetKeyCode(&fm_operator->state->phase));

	/* Modulate the phase. */
	/* The modulation is divided by two because up to two operators can provide modulation at once. */
	const unsigned int modulated_phase = (phase + phase_modulation / 2) & 0x3FF;

	/* Reduce the phase down to a single quarter of the span of a sine wave, since the other three quarters
	   are just mirrored anyway. This allows us to use a much smaller sine wave lookup table. */
	const cc_bool phase_is_in_negative_wave = (modulated_phase & 0x200) != 0;
	const cc_bool phase_is_in_mirrored_half_of_wave = (modulated_phase & 0x100) != 0;
	const unsigned int quarter_phase = (modulated_phase & 0xFF) ^ (phase_is_in_mirrored_half_of_wave ? 0xFF : 0);

	/* This table triples as a sine wave lookup table, a logarithm lookup table, and an attenuation lookup table.
	   The obtained attenuation is 12-bit. */
	const unsigned int phase_as_attenuation = fm_operator->constant->logarithmic_attenuation_sine_table[quarter_phase];

	/* Both attenuations are logarithms (measurements of decibels), so we can attenuate them by each other by just adding
	   them together instead of multiplying them. The result is a 13-bit value. */
	const unsigned int combined_attenuation = phase_as_attenuation + (attenuation << 2);

	/* Convert from logarithm (decibel) back to linear (sound pressure). */
	const int sample_absolute = InversePow2(fm_operator->constant, combined_attenuation);

	/* Restore the sign bit that we extracted earlier. */
	const int sample = (phase_is_in_negative_wave ? -sample_absolute : sample_absolute);

	/* Return the 14-bit sample. */
	return sample;
}
