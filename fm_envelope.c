#include "fm_envelope.h"

#include "clowncommon.h"

static unsigned int CalculateRate(const FM_Envelope_State *envelope, unsigned int key_code)
{
	unsigned int rate;

	if (envelope->rates[envelope->current_mode] == 0)
		rate = 0;
	else
		rate = CC_MIN(0x3F, envelope->rates[envelope->current_mode] * 2 + (key_code / envelope->key_scale));

	return rate;
}

void FM_Envelope_State_Initialise(FM_Envelope_State *state)
{
	/* Set envelope to update immediately. */
	state->countdown = 1;

	state->cycle_counter = 0;

	state->current_mode = FM_ENVELOPE_MODE_ATTACK;

	/* Silence channel. */
	FM_Envelope_SetTotalLevel(state, 0x7F);

	FM_Envelope_SetKeyScaleAndAttackRate(state, 0, 0);
}

cc_bool FM_Envelope_SetKeyOn(FM_Envelope_State *envelope, cc_bool key_on, unsigned int key_code)
{
	const cc_bool key_state_changed = envelope->key_on != key_on;

	/* An envelope cannot be key-on'd if it isn't key-off'd, and vice versa. */
	/* This is relied upon by Sonic's spring sound. */
	if (key_state_changed)
	{
		envelope->key_on = key_on;

		if (key_on)
		{
			envelope->current_mode = FM_ENVELOPE_MODE_ATTACK;

			if (CalculateRate(envelope, key_code) >= 0x1F * 2)
			{
				envelope->current_mode = FM_ENVELOPE_MODE_DECAY;
				envelope->current_attenuation = 0;
			}
		}
		else
		{
			envelope->current_mode = FM_ENVELOPE_MODE_RELEASE;
		}
	}

	return key_state_changed && key_on;
}

void FM_Envelope_SetTotalLevel(FM_Envelope_State *envelope, unsigned int total_level)
{
	/* Convert from 7-bit to 10-bit. */
	envelope->total_level = total_level << 3;
}

void FM_Envelope_SetKeyScaleAndAttackRate(FM_Envelope_State *envelope, unsigned int key_scale, unsigned int attack_rate)
{
	envelope->key_scale = 8 >> key_scale;
	envelope->rates[FM_ENVELOPE_MODE_ATTACK] = attack_rate;
}

void FM_Envelope_DecayRate(FM_Envelope_State *envelope, unsigned int decay_rate)
{
	envelope->rates[FM_ENVELOPE_MODE_DECAY] = decay_rate;
}

void FM_Envelope_SetSustainRate(FM_Envelope_State *envelope, unsigned int sustain_rate)
{
	envelope->rates[FM_ENVELOPE_MODE_SUSTAIN] = sustain_rate;
}

void FM_Envelope_SetSustainLevelAndReleaseRate(FM_Envelope_State *envelope, unsigned int sustain_level, unsigned int release_rate)
{
	envelope->sustain_level = sustain_level == 0xF ? 0x3E0 : sustain_level * 0x20;

	/* Convert from 4-bit to 5-bit to match the others. */
	envelope->rates[FM_ENVELOPE_MODE_RELEASE] = (release_rate << 1) | 1;
}

unsigned int FM_Envelope_Update(FM_Envelope_State *envelope, unsigned int key_code)
{
	if (--envelope->countdown == 0)
	{
		static const unsigned int cycle_bitmasks[0x40 / 4] = {
			#define GENERATE_BITMASK(x) ((1 << (x)) - 1)
			GENERATE_BITMASK(11),
			GENERATE_BITMASK(10),
			GENERATE_BITMASK(9),
			GENERATE_BITMASK(8),
			GENERATE_BITMASK(7),
			GENERATE_BITMASK(6),
			GENERATE_BITMASK(5),
			GENERATE_BITMASK(4),
			GENERATE_BITMASK(3),
			GENERATE_BITMASK(2),
			GENERATE_BITMASK(1),
			GENERATE_BITMASK(0),
			GENERATE_BITMASK(0),
			GENERATE_BITMASK(0),
			GENERATE_BITMASK(0),
			GENERATE_BITMASK(0)
			#undef GENERATE_BITMASK
		};

		const unsigned int rate = CalculateRate(envelope, key_code);

		envelope->countdown = 3;

		if ((envelope->cycle_counter++ & cycle_bitmasks[rate / 4]) == 0)
		{
			static const unsigned int deltas[0x40][8] = {
				{0, 0, 0, 0, 0, 0, 0, 0},
				{0, 0, 0, 0, 0, 0, 0, 0},
				{0, 1, 0, 1, 0, 1, 0, 1},
				{0, 1, 0, 1, 0, 1, 0, 1},
				{0, 1, 0, 1, 0, 1, 0, 1},
				{0, 1, 0, 1, 0, 1, 0, 1},
				{0, 1, 1, 1, 0, 1, 1, 1},
				{0, 1, 1, 1, 0, 1, 1, 1},
				{0, 1, 0, 1, 0, 1, 0, 1},
				{0, 1, 0, 1, 1, 1, 0, 1},
				{0, 1, 1, 1, 0, 1, 1, 1},
				{0, 1, 1, 1, 1, 1, 1, 1},
				{0, 1, 0, 1, 0, 1, 0, 1},
				{0, 1, 0, 1, 1, 1, 0, 1},
				{0, 1, 1, 1, 0, 1, 1, 1},
				{0, 1, 1, 1, 1, 1, 1, 1},
				{0, 1, 0, 1, 0, 1, 0, 1},
				{0, 1, 0, 1, 1, 1, 0, 1},
				{0, 1, 1, 1, 0, 1, 1, 1},
				{0, 1, 1, 1, 1, 1, 1, 1},
				{0, 1, 0, 1, 0, 1, 0, 1},
				{0, 1, 0, 1, 1, 1, 0, 1},
				{0, 1, 1, 1, 0, 1, 1, 1},
				{0, 1, 1, 1, 1, 1, 1, 1},
				{0, 1, 0, 1, 0, 1, 0, 1},
				{0, 1, 0, 1, 1, 1, 0, 1},
				{0, 1, 1, 1, 0, 1, 1, 1},
				{0, 1, 1, 1, 1, 1, 1, 1},
				{0, 1, 0, 1, 0, 1, 0, 1},
				{0, 1, 0, 1, 1, 1, 0, 1},
				{0, 1, 1, 1, 0, 1, 1, 1},
				{0, 1, 1, 1, 1, 1, 1, 1},
				{0, 1, 0, 1, 0, 1, 0, 1},
				{0, 1, 0, 1, 1, 1, 0, 1},
				{0, 1, 1, 1, 0, 1, 1, 1},
				{0, 1, 1, 1, 1, 1, 1, 1},
				{0, 1, 0, 1, 0, 1, 0, 1},
				{0, 1, 0, 1, 1, 1, 0, 1},
				{0, 1, 1, 1, 0, 1, 1, 1},
				{0, 1, 1, 1, 1, 1, 1, 1},
				{0, 1, 0, 1, 0, 1, 0, 1},
				{0, 1, 0, 1, 1, 1, 0, 1},
				{0, 1, 1, 1, 0, 1, 1, 1},
				{0, 1, 1, 1, 1, 1, 1, 1},
				{0, 1, 0, 1, 0, 1, 0, 1},
				{0, 1, 0, 1, 1, 1, 0, 1},
				{0, 1, 1, 1, 0, 1, 1, 1},
				{0, 1, 1, 1, 1, 1, 1, 1},
				{1, 1, 1, 1, 1, 1, 1, 1},
				{1, 1, 1, 2, 1, 1, 1, 2},
				{1, 2, 1, 2, 1, 2, 1, 2},
				{1, 2, 2, 2, 1, 2, 2, 2},
				{2, 2, 2, 2, 2, 2, 2, 2},
				{2, 2, 2, 4, 2, 2, 2, 4},
				{2, 4, 2, 4, 2, 4, 2, 4},
				{2, 4, 4, 4, 2, 4, 4, 4},
				{4, 4, 4, 4, 4, 4, 4, 4},
				{4, 4, 4, 8, 4, 4, 4, 8},
				{4, 8, 4, 8, 4, 8, 4, 8},
				{4, 8, 8, 8, 4, 8, 8, 8},
				{8, 8, 8, 8, 8, 8, 8, 8},
				{8, 8, 8, 8, 8, 8, 8, 8},
				{8, 8, 8, 8, 8, 8, 8, 8},
				{8, 8, 8, 8, 8, 8, 8, 8}
			};

			const unsigned int delta = deltas[rate][envelope->delta_index++ & 7];

			switch (envelope->current_mode)
			{
				case FM_ENVELOPE_MODE_ATTACK:
					envelope->current_attenuation += (~envelope->current_attenuation * delta) >> 4;
					envelope->current_attenuation &= 0x3FF;

					if (envelope->current_attenuation == 0)
						envelope->current_mode = FM_ENVELOPE_MODE_DECAY;

					break;

				case FM_ENVELOPE_MODE_DECAY:
					envelope->current_attenuation += delta;

					if (envelope->current_attenuation >= envelope->sustain_level)
						envelope->current_mode = FM_ENVELOPE_MODE_SUSTAIN;

					break;

				case FM_ENVELOPE_MODE_SUSTAIN:
				case FM_ENVELOPE_MODE_RELEASE:
					envelope->current_attenuation += delta;

					if (envelope->current_attenuation > 0x3FF)
						envelope->current_attenuation = 0x3FF;

					break;
			}
		}
	}

	return CC_MIN(0x3FF, envelope->current_attenuation + envelope->total_level);
}
