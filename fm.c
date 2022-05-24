/*
TODO:

CSM Mode:
http://gendev.spritesmind.net/forum/viewtopic.php?p=5650#p5650

Differences between YM2612 and YM2608:
http://gendev.spritesmind.net/forum/viewtopic.php?p=5680#p5680

Timing of timer counter updates:
http://gendev.spritesmind.net/forum/viewtopic.php?p=5687#p5687

Test register that makes every channel output the DAC sample:
http://gendev.spritesmind.net/forum/viewtopic.php?t=1118

How the envelope generator works:
http://gendev.spritesmind.net/forum/viewtopic.php?p=5716#p5716
http://gendev.spritesmind.net/forum/viewtopic.php?p=6224#p6224
http://gendev.spritesmind.net/forum/viewtopic.php?p=6522#p6522

Sine table resolution, DAC and FM operator mixing/volume/clipping:
http://gendev.spritesmind.net/forum/viewtopic.php?p=5958#p5958

Maybe I should implement multiplexing instead of mixing the 6 channel together?
Multiplexing is more authentic, but is mixing *better*?

FM and PSG balancing:
http://gendev.spritesmind.net/forum/viewtopic.php?p=5960#p5960:

Operator output caching during algorithm stage:
http://gendev.spritesmind.net/forum/viewtopic.php?p=6090#p6090

Self-feedback patents:
http://gendev.spritesmind.net/forum/viewtopic.php?p=6096#p6096

How the operator unit works:
http://gendev.spritesmind.net/forum/viewtopic.php?p=6114#p6114

How the phase generator works:
http://gendev.spritesmind.net/forum/viewtopic.php?p=6177#p6177

Some details about the DAC:
http://gendev.spritesmind.net/forum/viewtopic.php?p=6258#p6258

Operator unit value recycling:
http://gendev.spritesmind.net/forum/viewtopic.php?p=6287#p6287
http://gendev.spritesmind.net/forum/viewtopic.php?p=6333#p6333

Some details on the internal tables:
http://gendev.spritesmind.net/forum/viewtopic.php?p=6741#p6741

LFO locking bug:
http://gendev.spritesmind.net/forum/viewtopic.php?p=7490#p7490

Multiplexing details:
http://gendev.spritesmind.net/forum/viewtopic.php?p=7509#p7509

Multiplexing ordering:
http://gendev.spritesmind.net/forum/viewtopic.php?p=7516#p7516

Corrected and expanded envelope generator stuff:
http://gendev.spritesmind.net/forum/viewtopic.php?p=7967#p7967

Some miscellaneous details (there's a bunch of minor stuff after it too):
http://gendev.spritesmind.net/forum/viewtopic.php?p=7999#p7999

YM3438 differences:
http://gendev.spritesmind.net/forum/viewtopic.php?p=8406#p8406

Phase generator steps:
http://gendev.spritesmind.net/forum/viewtopic.php?p=8908#p8908

Simulating the multiplex with addition:
http://gendev.spritesmind.net/forum/viewtopic.php?p=10441#p10441

DAC precision loss:
http://gendev.spritesmind.net/forum/viewtopic.php?p=11873#p11873

DAC and FM6 updating:
http://gendev.spritesmind.net/forum/viewtopic.php?p=14105#p14105

Some questions:
http://gendev.spritesmind.net/forum/viewtopic.php?p=18723#p18723

Debug register and die shot analysis:
http://gendev.spritesmind.net/forum/viewtopic.php?p=26964#p26964

The dumb busy flag:
http://gendev.spritesmind.net/forum/viewtopic.php?p=27124#p27124

Control unit (check stuff afterwards too):
http://gendev.spritesmind.net/forum/viewtopic.php?p=29471#p29471

More info about one of the test registers:
http://gendev.spritesmind.net/forum/viewtopic.php?p=30065#p30065

LFO queries:
http://gendev.spritesmind.net/forum/viewtopic.php?p=30935#p30935

More test register stuff:
http://gendev.spritesmind.net/forum/viewtopic.php?p=31285#p31285

And of course there's Nuked, which will answer all questions once and for all:
https://github.com/nukeykt/Nuked-OPN2

*/

#include "fm.h"

#include <math.h>

#include "clowncommon.h"

#include "error.h"

void FM_Constant_Initialise(FM_Constant *constant)
{
	FM_Operator_Constant_Initialise(&constant->operators);
}

void FM_State_Initialise(FM_State *state)
{
	FM_Channel *channel;

	/* Make the channels silent. */
	for (channel = &state->channels[0]; channel < &state->channels[CC_COUNT_OF(state->channels)]; ++channel)
	{
		FM_Operator *operator;

		for (operator = &channel->operators[0]; operator < &channel->operators[CC_COUNT_OF(channel->operators)]; ++operator)
		{
			FM_Phase_Initialise(&operator->phase);
			operator->attenuation = 0x7F;
		}

		/* The fourth operator is always a slot. */
		channel->operators[3].is_slot = cc_true;
	}

	state->dac_sample = 0;
	state->dac_enabled = cc_false;
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
					/* TODO - Check what happens if you try to access the 'gap' channels on real hardware. */
					static const unsigned int table[] = {0, 1, 2, 0, 3, 4, 5, 0};
					FM_Channel* const channel = &fm->state->channels[table[data & 7]];

					channel->key_on = (data & 0xF0) != 0;

					break;
				}

				case 0x2A:
					/* DAC sample. */
					/* Convert from unsigned 8-bit PCM to signed 16-bit PCM. */
					fm->state->dac_sample = ((int)data - 0x80) * 0x100;
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
		const unsigned int channel_index = fm->state->address & 3;
		FM_Channel* const channel = &fm->state->channels[fm->state->port + channel_index];

		/* There is no fourth channel per slot. */
		/* TODO: See how real hardware handles this. */
		if (channel_index != 3)
		{
			if (fm->state->address < 0xA0)
			{
				/* Per-operator. */
				FM_Operator* const operator = &channel->operators[(fm->state->address >> 2) & 3];

				switch (fm->state->address / 0x10)
				{
					default:
						PrintError("Unrecognised FM address latched (0x%02X)", fm->state->address);
						break;

					case 0x50 / 0x10:
					case 0x60 / 0x10:
					case 0x70 / 0x10:
					case 0x80 / 0x10:
					case 0x90 / 0x10:
						break;

					case 0x30 / 0x10:
						/* Detune and multiplier. */
						FM_Phase_SetDetuneAndMultiplier(&operator->phase, data);

						break;

					case 0x40 / 0x10:
						/* Volume attenuation. */
						operator->attenuation = data & 0x7F;
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
						break;

					case 0xA0 / 4:
					{
						/* Frequency low bits. */
						const unsigned long f_number_and_block = data | (channel->cached_upper_frequency_bits << 8);

						FM_Operator *operator;

						for (operator = &channel->operators[0]; operator < &channel->operators[CC_COUNT_OF(channel->operators)]; ++operator)
							FM_Phase_SetFrequency(&operator->phase, f_number_and_block);

						break;
					}

					case 0xA4 / 4:
						/* Frequency high bits. */
						/* http://gendev.spritesmind.net/forum/viewtopic.php?p=5621#p5621 */
						channel->cached_upper_frequency_bits = data;
						break;

					case 0xB0 / 4:
						/* Feedback and algorithm. */
						switch (data & 7)
						{
							case 0:
							case 1:
							case 2:
							case 3:
								channel->operators[0].is_slot = cc_false;
								channel->operators[1].is_slot = cc_false;
								channel->operators[2].is_slot = cc_false;
								break;

							case 4:
								channel->operators[0].is_slot = cc_false;
								channel->operators[1].is_slot = cc_false;
								channel->operators[2].is_slot = cc_true;
								break;

							case 5:
							case 6:
								channel->operators[0].is_slot = cc_false;
								channel->operators[1].is_slot = cc_true;
								channel->operators[2].is_slot = cc_true;
								break;

							case 7:
								channel->operators[0].is_slot = cc_true;
								channel->operators[1].is_slot = cc_true;
								channel->operators[2].is_slot = cc_true;
								break;
						}

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
		const unsigned int left_mask = channel->pan_left ? -1 : 0;
		const unsigned int right_mask = channel->pan_right ? -1 : 0;

		short *sample_buffer_pointer;

		if (channel == &fm->state->channels[5] && fm->state->dac_enabled)
		{
			/* DAC sample. */
			const int sample = fm->state->dac_sample;
			const int left_sample = sample & left_mask;
			const int right_sample = sample & right_mask;

			sample_buffer_pointer = sample_buffer;

			while (sample_buffer_pointer != sample_buffer_end)
			{
				/* Output sample. */
				*sample_buffer_pointer++ += left_sample;
				*sample_buffer_pointer++ += right_sample;
			}
		}
		else
		{
			if (channel->key_on)
			{
				/* FM sample. */
				FM_Operator *operator;

				for (operator = &channel->operators[0]; operator < &channel->operators[CC_COUNT_OF(channel->operators)]; ++operator)
				{
					if (operator->is_slot)
					{
						sample_buffer_pointer = sample_buffer;

						while (sample_buffer_pointer != sample_buffer_end)
						{
							/* Obtain sample. */
							const int raw_sample = FM_Operator_Process(&fm->constant->operators, FM_Phase_Increment(&operator->phase), 0, operator->attenuation);

							/* The sample is 14-bit, so make it 10-bit so that it can be mixed with the other five channels without clipping. */
							const int sample = raw_sample / (1 << 4);

							/* Output sample. */
							*sample_buffer_pointer++ += sample & left_mask;
							*sample_buffer_pointer++ += sample & right_mask;
						}
					}
				}
			}
		}
	}
}
