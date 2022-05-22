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
	fm->state->port = port * 3;
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
					PrintError("Unrecognised FM address latched (0x%02X)", fm->state->address);
					break;

				case 0x22:
				case 0x24:
				case 0x25:
				case 0x26:
				case 0x27:
					break;

				case 0x28:
				{
					/* Key on/off. */
					/* There's a gap between channels 3 and 4. */
					/* TODO - Check what happens if you try to access the 'gap' channel on real hardware. */
					static const unsigned int table[] = {0, 1, 2, 0, 3, 4, 5, 0};
					FM_Channel* const channel = &fm->state->channels[table[data & 7]];

					channel->key_on = (data & 0xF0) != 0;

					break;
				}

				case 0x2A:
					/* DAC sample. */
					/* Convert from unsigned 8-bit PCM to signed 16-bit PCM. */
					fm->state->dac_sample = (data - 0x80) * 0x100;
					break;

				case 0x2B:
					/* DAC enable/disable. */
					fm->state->dac_enabled = (data & 0x80) != 0;
					break;
			}
		}
	}
	else
	{
		const unsigned int channel_index = fm->state->address % 4;
		FM_Channel* const channel = &fm->state->channels[fm->state->port + channel_index];

		/* There is no fourth channel per slot. */
		/* TODO: See how real hardware handles this. */
		if (channel_index != 3)
		{
			if (fm->state->address < 0xA0)
			{
				/* Per-operator. */
				FM_Operator* const operator = &channel->operators[fm->state->address / 4];

				switch (fm->state->address / 0x10)
				{
					default:
						PrintError("Unrecognised FM address latched (0x%02X)", fm->state->address);
						break;

					case 0x30 / 0x10:
					case 0x50 / 0x10:
					case 0x60 / 0x10:
					case 0x70 / 0x10:
					case 0x80 / 0x10:
					case 0x90 / 0x10:
						break;

					case 0x40 / 0x10:
						/* Volume attenuation. */
						operator->attenuation = data & 0x7F;

						/* TODO: Temporary. */
						channel->attenuation = data & 0x7F;

						break;
				}
			}
			else
			{
				/* Per-channel. */
				switch (fm->state->address / 4)
				{
					default:
						PrintError("Unrecognised FM address latched (0x%02X)", fm->state->address);
						break;

					case 0xA8 / 4:
					case 0xAC / 4:
					case 0xB0 / 4:
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

					case 0xB4 / 4:
						/* Panning, AMS, FMS. */
						channel->pan_left = (data & 0x80) != 0;
						channel->pan_right = (data & 0x40) != 0;
						break;
				}
			}
		}
	}
}

void FM_Update(const FM *fm, short *sample_buffer, size_t total_frames)
{
	const short* const sample_buffer_end = &sample_buffer[total_frames * 2];

	FM_Channel *channel;

	for (channel = &fm->state->channels[0]; channel < &fm->state->channels[CC_COUNT_OF(fm->state->channels)]; ++channel)
	{
		if (channel->key_on)
		{
			short *sample_buffer_pointer = sample_buffer;

			if (channel == &fm->state->channels[5] && fm->state->dac_enabled)
			{
				/* DAC sample. */
				const int sample = fm->state->dac_sample;

				while (sample_buffer_pointer != sample_buffer_end)
				{
					/* Output sample. */
					if (channel->pan_left)
						*sample_buffer_pointer += sample;

					++sample_buffer_pointer;

					if (channel->pan_right)
						*sample_buffer_pointer += sample;

					++sample_buffer_pointer;
				}
			}
			else
			{
				const short *sine_wave = fm->constant->sine_waves[channel->attenuation];

				/* FM sample. */
				while (sample_buffer_pointer != sample_buffer_end)
				{
					/* The frequency measures the span of the sine wave as 0x100000, but the lookup table is only 0x1000 values long,
					   so calculate the scale between them, and use it to convert the position to an index into the table. */
					const unsigned long sine_wave_index = channel->sine_wave_position / (0x100000 / LENGTH_OF_SINE_WAVE_LOOKUP_TABLE);

					/* Obtain sample. */
					const short sample = sine_wave[sine_wave_index % LENGTH_OF_SINE_WAVE_LOOKUP_TABLE];

					/* Output sample. */
					if (channel->pan_left)
						*sample_buffer_pointer += sample;

					++sample_buffer_pointer;

					if (channel->pan_right)
						*sample_buffer_pointer += sample;

					++sample_buffer_pointer;

					/* Advance by one step. */
					channel->sine_wave_position += channel->sine_wave_step;
				}
			}
		}
	}
}
