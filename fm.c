#include "fm.h"

#include <math.h>

#include "clowncommon.h"

#include "error.h"

void FM_Constant_Initialise(FM_Constant *constant)
{
	unsigned long i;
	unsigned int j;

	/* Compute the sine wave lookup table. */
	for (i = 0; i < LENGTH_OF_SINE_WAVE_LOOKUP_TABLE; ++i)
	{
		/* Iterate over the entire length of the sine wave. */
		const double sine_index = (double)i / (double)LENGTH_OF_SINE_WAVE_LOOKUP_TABLE * (CC_PI * 2.0);

		/* '0x7FFF' is the highest value that can be represented by S16 audio.
		   It is then divided by the number of channels to prevent audio clipping. */
		const double sample_max = (double)0x7FFF / 6.0;

		constant->sine_waves[0][i] = (short)(sample_max * sin(sine_index));
	}

	/* Compute the attenuated wave lookup tables (used for volume control). */
	/* Notably, the attenuation increases by 3/4 of a decibel every time. */
	for (j = 1; j < CC_COUNT_OF(constant->sine_waves); ++j)
	{
		const double scale = pow(10.0, -0.75 * (double)j / 20.0);

		for (i = 0; i < LENGTH_OF_SINE_WAVE_LOOKUP_TABLE; ++i)
			constant->sine_waves[j][i] = (short)((double)constant->sine_waves[0][i] * scale);
	}
}

void FM_State_Initialise(FM_State *state)
{
	FM_Channel *channel;

	/* Make the channels silent. */
	for (channel = &state->channels[0]; channel < &state->channels[CC_COUNT_OF(state->channels)]; ++channel)
	{
		channel->sine_wave_position = 0;
		channel->sine_wave_step = 0;
		channel->attenuation = 0x7F;
	}
}

void FM_DoAddress(const FM *fm, unsigned int port, unsigned int address)
{
	fm->state->port = port;
	fm->state->address = address;
}

void FM_DoData(const FM *fm, unsigned int data)
{
	if (fm->state->address < 0x30)
	{
		if (fm->state->port == 0)
		{
			switch (fm->state->address)
			{
				default:
					PrintError("Unrecognised FM address latched (0x%2X)");
					break;

				case 0x22:
				case 0x24:
				case 0x25:
				case 0x26:
				case 0x27:
				case 0x2A:
				case 0x2B:
					break;
			}
		}
	}
	else
	{
		FM_Channel* const channel = &fm->state->channels[fm->state->port * 3 + fm->state->address % 4];

		switch (fm->state->address / 4)
		{
			default:
				PrintError("Unrecognised FM address latched (0x%2X)");
				break;

			case 0x30 / 4:
			case 0x34 / 4:
			case 0x38 / 4:
			case 0x3C / 4:
			case 0x50 / 4:
			case 0x60 / 4:
			case 0x70 / 4:
			case 0x80 / 4:
			case 0x90 / 4:
			case 0x94 / 4:
			case 0x98 / 4:
			case 0x9C / 4:
			case 0xA8 / 4:
			case 0xAC / 4:
			case 0xB0 / 4:
			case 0xB4 / 4:
				break;

			case 0x40 / 4:
				/* Volume attenuation. */
				channel->attenuation = data & 0x7F;
				break;

			case 0xA0 / 4:
			{
				/* Frequency low bits. */
				const unsigned int frequency_high_bits = fm->state->cached_data & 7;
				const unsigned int frequency_shift = (fm->state->cached_data >> 3) & 7;

				channel->sine_wave_step = (data | (frequency_high_bits << 8)) << frequency_shift;

				break;
			}

			case 0xA4 / 4:
				/* Frequency high bits. */
				/* Apparently these high bits must be written before the low bits, so this optimisation is safe. */
				fm->state->cached_data = data;
				break;
		}
	}
}

void FM_Update(const FM *fm, short *sample_buffer, size_t total_frames)
{
	FM_Channel *channel;

	for (channel = &fm->state->channels[0]; channel < &fm->state->channels[CC_COUNT_OF(fm->state->channels)]; ++channel)
	{
		short *sample_buffer_pointer = sample_buffer;

		while (sample_buffer_pointer != &sample_buffer[total_frames * 2])
		{
			/* The frequency measures the span of the sine wave as 0x100000, but the lookup table is only 0x1000 values long,
			   so calculate the scale between them, and use it to convert the position to an index into the table. */
			const unsigned long sine_wave_index = channel->sine_wave_position / (0x100000 / LENGTH_OF_SINE_WAVE_LOOKUP_TABLE);

			/* Obtain sample. */
			const short sample = fm->constant->sine_waves[channel->attenuation][sine_wave_index % LENGTH_OF_SINE_WAVE_LOOKUP_TABLE];

			/* Output it. */
			*sample_buffer_pointer++ += sample;
			*sample_buffer_pointer++ += sample;

			/* Advance by one step. */
			channel->sine_wave_position += channel->sine_wave_step;
		}
	}
}
