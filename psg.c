/*
PSG emulator

This webpage has been an invaluable resource:
https://www.smspower.org/Development/SN76489
*/

#include "psg.h"

#include <math.h> /* TODO - Temporary */

#include "clowncommon.h"

void PSG_Init(PSG_State *state)
{
	unsigned int i;

	/* Reset tone channels */
	for (i = 0; i < CC_COUNT_OF(state->tones); ++i)
	{
		state->tones[i].countdown = 0;
		state->tones[i].countdown_master = 0;
		state->tones[i].attenuation = 0xF; /* Silence the channels on startup */
		state->tones[i].output_bit = 0;
	}

	/* Reset noise channel */
	state->noise.countdown = 0;
	state->noise.attenuation = 0xF;
	state->noise.fake_output_bit = 0;
	state->noise.real_output_bit = 0;
	state->noise.frequency_mode = 0;
	state->noise.white_noise_mode = cc_false;
	state->noise.shift_register = 0;

	/* Reset the latched command data */
	state->latched_command.channel = 0;
	state->latched_command.is_volume_command = cc_false;

	/* Generate the volume lookup table */
	/* TODO - Temporary */
	for (i = 0; i < 0xF; ++i)
	{
		/* Each volume level is 2 decibels lower than the last */
		const int volume = ((float)0x7FFF / 4.0f) * powf(10.0f, -2.0f * (float)i / 20.0f);

		state->volumes[i][0] = volume; /* Positive phase */
		state->volumes[i][1] = -volume; /* Negative phase */
	}

	/* The lowest volume is 0 */
	state->volumes[0xF][0] = 0;
	state->volumes[0xF][1] = 0;
}

void PSG_DoCommand(PSG_State *state, unsigned int command)
{
	const cc_bool latch = !!(command & 0x80);

	if (latch)
	{
		/* Latch command */
		state->latched_command.channel = (command >> 5) & 3;
		state->latched_command.is_volume_command = !!(command & 0x10);
	}

	if (state->latched_command.channel < CC_COUNT_OF(state->tones))
	{
		/* Tone channel */
		PSG_ToneState *tone = &state->tones[state->latched_command.channel];

		if (state->latched_command.is_volume_command)
		{
			/* Volume command */
			tone->attenuation = command & 0xF;
			/* According to http://md.railgun.works/index.php?title=PSG, this should happen,
			   but when I test it, I get crackly audio, so I've disabled it for now. */
			   /*tone->output_bit = 0;*/
		}
		else
		{
			/* Frequency command */
			if (latch)
			{
				/* Low frequency bits */
				tone->countdown_master &= ~0xF;
				tone->countdown_master |= command & 0xF;
			}
			else
			{
				/* High frequency bits */
				tone->countdown_master &= 0xF;
				tone->countdown_master |= (command & 0x3F) << 4;
			}
		}
	}
	else
	{
		/* Noise channel */
		if (state->latched_command.is_volume_command)
		{
			/* Volume command */
			state->noise.attenuation = command & 0xF;
			/* According to http://md.railgun.works/index.php?title=PSG, this should happen,
			   but when I test it, I get crackly audio, so I've disabled it for now. */
			   /*state->noise.fake_output_bit = 0;*/
		}
		else
		{
			/* Frequency command */
			state->noise.white_noise_mode = !!(command & 4);
			state->noise.frequency_mode = command & 3;

			/* https://www.smspower.org/Development/SN76489
			   "When the noise register is written to, the shift register is reset,
			   such that all bits are zero except for the highest bit. This will make
			   the "periodic noise" output a 1/16th (or 1/15th) duty cycle, and is
			   important as it also affects the sound of white noise." */
			state->noise.shift_register = 1;
		}
	}
}

void PSG_Update(PSG_State *state, short *sample_buffer, size_t total_samples)
{
	unsigned int i;
	size_t j;
	short *sample_buffer_pointer;

	/* Do the tone channels */
	for (i = 0; i < CC_COUNT_OF(state->tones); ++i)
	{
		PSG_ToneState *tone = &state->tones[i];

		sample_buffer_pointer = sample_buffer;

		for (j = 0; j < total_samples; ++j)
		{
			/* This countdown is responsible for the channel's frequency */
			if (tone->countdown-- == 0)
			{
				/* Reset the countdown */
				tone->countdown = tone->countdown_master;

				/* Switch from positive phase to negative phase and vice versa */
				tone->output_bit = !tone->output_bit;
			}

			/* Output a sample */
			*sample_buffer_pointer++ += state->volumes[tone->attenuation][tone->output_bit];
		}
	}

	/* Do the noise channel */
	sample_buffer_pointer = sample_buffer;

	for (j = 0; j < total_samples; ++j)
	{
		/* This countdown is responsible for the channel's frequency */
		if (state->noise.countdown-- == 0)
		{
			/* Reset the countdown */
			switch (state->noise.frequency_mode)
			{
				case 0:
					state->noise.countdown = 0x10;
					break;

				case 1:
					state->noise.countdown = 0x20;
					break;

				case 2:
					state->noise.countdown = 0x40;
					break;

				case 3:
					/* Use the last tone channel's frequency */
					state->noise.countdown = state->tones[CC_COUNT_OF(state->tones) - 1].countdown_master;
					break;
			}

			state->noise.fake_output_bit = !state->noise.fake_output_bit;

			if (state->noise.fake_output_bit)
			{
				/* The noise channel works by maintaining a 16-bit register, whose bits are rotated every time
				   the output bit goes from low to high. The bit that was rotated from the 'bottom' of the
				   register to the 'top' is what is output to the speaker. In white noise mode, after rotation,
				   the bit at the 'top' is XOR'd with the bit that is third from the 'bottom'. */
				state->noise.real_output_bit = (state->noise.shift_register & 0x8000) >> 15;

				state->noise.shift_register <<= 1;
				state->noise.shift_register |= state->noise.real_output_bit;

				if (state->noise.white_noise_mode)
					state->noise.shift_register ^= (state->noise.shift_register & 0x2000) >> 13;
			}
		}

		/* Output a sample */
		*sample_buffer_pointer++ += state->volumes[state->noise.attenuation][state->noise.real_output_bit];
	}
}
