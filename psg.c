/*
PSG emulator

This webpage has been an invaluable resource:
https://www.smspower.org/Development/SN76489
*/

#include "psg.h"

#include <math.h>

#include "clowncommon.h"

void PSG_Constant_Initialise(PSG_Constant *constant)
{
	unsigned int i;

	/* Generate the volume lookup table. */
	for (i = 0; i < 0xF; ++i)
	{
		/* Each volume level is 2 decibels lower than the last. */
		/* The division by 4 is because there are 4 channels, so we want to prevent audio clipping. */
		const short volume = (short)(((double)0x7FFF / 4.0) * pow(10.0, -2.0 * (double)i / 20.0));

		constant->volumes[i][0] = volume; /* Positive phase. */
		constant->volumes[i][1] = -volume; /* Negative phase. */
	}

	/* The lowest volume is always 0. */
	constant->volumes[0xF][0] = 0;
	constant->volumes[0xF][1] = 0;
}

void PSG_State_Initialise(PSG_State *state)
{
	unsigned int i;

	/* Reset tone channels. */
	for (i = 0; i < CC_COUNT_OF(state->tones); ++i)
	{
		state->tones[i].countdown = 0;
		state->tones[i].countdown_master = 0;
		state->tones[i].attenuation = 0xF; /* Silence the channels on startup. */
		state->tones[i].output_bit = 0;
	}

	/* Reset noise channel. */
	state->noise.countdown = 0;
	state->noise.attenuation = 0xF;
	state->noise.fake_output_bit = 0;
	state->noise.real_output_bit = 0;
	state->noise.frequency_mode = 0;
	state->noise.type = PSG_NOISE_TYPE_PERIODIC;
	state->noise.shift_register = 0;

	/* Reset the latched command data. */
	state->latched_command.channel = 0;
	state->latched_command.is_volume_command = cc_false;
}

void PSG_DoCommand(const PSG *psg, unsigned int command)
{
	const cc_bool latch = (command & 0x80) != 0;

	if (latch)
	{
		/* Latch command. */
		psg->state->latched_command.channel = (command >> 5) & 3;
		psg->state->latched_command.is_volume_command = (command & 0x10) != 0;
	}

	if (psg->state->latched_command.channel < CC_COUNT_OF(psg->state->tones))
	{
		/* Tone channel. */
		PSG_ToneState *tone = &psg->state->tones[psg->state->latched_command.channel];

		if (psg->state->latched_command.is_volume_command)
		{
			/* Volume attenuation command. */
			tone->attenuation = command & 0xF;
			/* According to http://md.railgun.works/index.php?title=PSG, this should happen,
			   but when I test it, I get crackly audio, so I've disabled it for now. */
			/*tone->output_bit = 0;*/
		}
		else
		{
			/* Frequency command. */
			if (latch)
			{
				/* Low frequency bits. */
				tone->countdown_master &= ~0xF;
				tone->countdown_master |= command & 0xF;
			}
			else
			{
				/* High frequency bits. */
				tone->countdown_master &= 0xF;
				tone->countdown_master |= (command & 0x3F) << 4;
			}
		}
	}
	else
	{
		/* Noise channel. */
		if (psg->state->latched_command.is_volume_command)
		{
			/* Volume attenuation command. */
			psg->state->noise.attenuation = command & 0xF;
			/* According to http://md.railgun.works/index.php?title=PSG, this should happen,
			   but when I test it, I get crackly audio, so I've disabled it for now. */
			/*state->noise.fake_output_bit = 0;*/
		}
		else
		{
			/* Frequency and noise type command. */
			psg->state->noise.type = (command & 4) ? PSG_NOISE_TYPE_WHITE : PSG_NOISE_TYPE_PERIODIC;
			psg->state->noise.frequency_mode = command & 3;

			/* https://www.smspower.org/Development/SN76489
			   "When the noise register is written to, the shift register is reset,
			   such that all bits are zero except for the highest bit. This will make
			   the "periodic noise" output a 1/16th (or 1/15th) duty cycle, and is
			   important as it also affects the sound of white noise." */
			psg->state->noise.shift_register = 1;
		}
	}
}

void PSG_Update(const PSG *psg, short *sample_buffer, size_t total_samples)
{
	unsigned int i;
	size_t j;
	short *sample_buffer_pointer;

	/* Do the tone channels. */
	for (i = 0; i < CC_COUNT_OF(psg->state->tones); ++i)
	{
		PSG_ToneState *tone = &psg->state->tones[i];

		sample_buffer_pointer = sample_buffer;

		for (j = 0; j < total_samples; ++j)
		{
			/* This countdown is responsible for the channel's frequency. */
			if (tone->countdown-- == 0)
			{
				/* Reset the countdown. */
				tone->countdown = tone->countdown_master;

				/* Switch from positive phase to negative phase and vice versa. */
				tone->output_bit = !tone->output_bit;
			}

			/* Output a sample. */
			*sample_buffer_pointer++ += psg->constant->volumes[tone->attenuation][tone->output_bit];
		}
	}

	/* Do the noise channel. */
	sample_buffer_pointer = sample_buffer;

	for (j = 0; j < total_samples; ++j)
	{
		/* This countdown is responsible for the channel's frequency. */
		if (psg->state->noise.countdown-- == 0)
		{
			/* Reset the countdown. */
			switch (psg->state->noise.frequency_mode)
			{
				case 0:
					psg->state->noise.countdown = 0x10 - 1;
					break;

				case 1:
					psg->state->noise.countdown = 0x20 - 1;
					break;

				case 2:
					psg->state->noise.countdown = 0x40 - 1;
					break;

				case 3:
					/* Use the last tone channel's frequency. */
					psg->state->noise.countdown = psg->state->tones[CC_COUNT_OF(psg->state->tones) - 1].countdown_master;
					break;
			}

			psg->state->noise.fake_output_bit = !psg->state->noise.fake_output_bit;

			if (psg->state->noise.fake_output_bit)
			{
				/* The noise channel works by maintaining a 16-bit register, whose bits are rotated every time
				   the output bit goes from low to high. The bit that was rotated from the 'bottom' of the
				   register to the 'top' is what is output to the speaker. In white noise mode, after rotation,
				   the bit at the 'top' is XOR'd with the bit that is third from the 'bottom'. */
				psg->state->noise.real_output_bit = (psg->state->noise.shift_register & 0x8000) >> 15;

				psg->state->noise.shift_register <<= 1;
				psg->state->noise.shift_register |= psg->state->noise.real_output_bit;

				if (psg->state->noise.type == PSG_NOISE_TYPE_WHITE)
					psg->state->noise.shift_register ^= (psg->state->noise.shift_register & 0x2000) >> 13;
			}
		}

		/* Output a sample. */
		*sample_buffer_pointer++ += psg->constant->volumes[psg->state->noise.attenuation][psg->state->noise.real_output_bit];
	}
}
