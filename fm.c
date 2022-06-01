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

The 9th DAC sample bit.

*/

/* 8 is chosen because there are 6 FM channels, of which the DAC can replace one of, as well at the PSG.
   The PSG with all of its channels at maximum volume reaches the volume of a single FM channel at maximum.
   Technically, this means that 7 is a more appropriate number than 8. However, dividing by 8 is simpler
   than dividing by 7, so that was opted for instead. */
#define VOLUME_DIVIDER 8

#include "fm.h"

#include <math.h>

#include "clowncommon.h"

#include "error.h"

void FM_Constant_Initialise(FM_Constant *constant)
{
	FM_Channel_Constant_Initialise(&constant->channels);
}

void FM_State_Initialise(FM_State *state)
{
	FM_ChannelMetadata *channel;

	for (channel = &state->channels[0]; channel < &state->channels[CC_COUNT_OF(state->channels)]; ++channel)
	{
		/* Panning must be enabled by default. Without this, Sonic 1's 'Sega' chant doesn't play. */
		channel->pan_left = cc_true;
		channel->pan_right = cc_true;

		FM_Channel_State_Initialise(&channel->state);
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
					FM_ChannelMetadata* const channel_metadata = &fm->state->channels[table[data & 7]];
					const FM_Channel channel = {&fm->constant->channels, &channel_metadata->state};

					FM_Channel_SetKeyOn(&channel, 0, (data & (1 << 4)) != 0);
					FM_Channel_SetKeyOn(&channel, 2, (data & (1 << 5)) != 0);
					FM_Channel_SetKeyOn(&channel, 1, (data & (1 << 6)) != 0);
					FM_Channel_SetKeyOn(&channel, 3, (data & (1 << 7)) != 0);

					break;
				}

				case 0x2A:
					/* DAC sample. */
					/* Convert from unsigned 8-bit PCM to signed 16-bit PCM. */
					fm->state->dac_sample = ((int)data - 0x80) * (0x100 / VOLUME_DIVIDER);
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
		FM_ChannelMetadata* const channel_metadata = &fm->state->channels[fm->state->port + channel_index];
		const FM_Channel channel = {&fm->constant->channels, &channel_metadata->state};

		/* There is no fourth channel per slot. */
		/* TODO: See how real hardware handles this. */
		if (channel_index != 3)
		{
			if (fm->state->address < 0xA0)
			{
				/* Per-operator. */
				const unsigned int operator_index = (fm->state->address >> 2) & 3;

				switch (fm->state->address / 0x10)
				{
					default:
						PrintError("Unrecognised FM address latched (0x%02X)", fm->state->address);
						break;

					case 0x30 / 0x10:
						/* Detune and multiplier. */
						FM_Channel_SetDetuneAndMultiplier(&channel, operator_index, (data >> 4) & 7, data & 0xF);
						break;

					case 0x40 / 0x10:
						/* Total level. */
						FM_Channel_SetTotalLevel(&channel, operator_index, data & 0x7F);
						break;

					case 0x50 / 0x10:
						/* Key scale and attack rate. */
						FM_Channel_SetKeyScaleAndAttackRate(&channel, operator_index, (data >> 6) & 3, data & 0x1F);
						break;

					case 0x60 / 0x10:
						/* Amplitude modulation on and decay rate. */
						/* TODO: LFO. */
						FM_Channel_DecayRate(&channel, operator_index, data & 0x1F);
						break;

					case 0x70 / 0x10:
						/* Sustain rate. */
						FM_Channel_SetSustainRate(&channel, operator_index, data & 0x1F);
						break;

					case 0x80 / 0x10:
						/* Sustain level and release rate. */
						FM_Channel_SetSustainLevelAndReleaseRate(&channel, operator_index, (data >> 4) & 0xF, data & 0xF);
						break;

					case 0x90 / 0x10:
						/* TODO: SSG-EG. */
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
						/* Frequency low bits. */
						FM_Channel_SetFrequency(&channel, data | (channel_metadata->cached_upper_frequency_bits << 8));
						break;

					case 0xA4 / 4:
						/* Frequency high bits. */
						/* http://gendev.spritesmind.net/forum/viewtopic.php?p=5621#p5621 */
						channel_metadata->cached_upper_frequency_bits = data & 0x3F;
						break;

					case 0xB0 / 4:
						FM_Channel_SetFeedbackAndAlgorithm(&channel, (data >> 3) & 7, data & 7);
						break;

					case 0xB4 / 4:
						/* Panning, AMS, FMS. */
						channel_metadata->pan_left = (data & 0x80) != 0;
						channel_metadata->pan_right = (data & 0x40) != 0;
						break;
				}
			}
		}
	}
}

void FM_Update(const FM *fm, short *sample_buffer, size_t total_frames)
{
	const short* const sample_buffer_end = &sample_buffer[total_frames * 2];

	FM_ChannelMetadata *channel_metadata;

	for (channel_metadata = &fm->state->channels[0]; channel_metadata < &fm->state->channels[CC_COUNT_OF(fm->state->channels)]; ++channel_metadata)
	{
		const FM_Channel channel = {&fm->constant->channels, &channel_metadata->state};

		/* These will be used in later boolean algebra, to avoid branches. */
		const unsigned int left_mask = channel_metadata->pan_left ? -1 : 0;
		const unsigned int right_mask = channel_metadata->pan_right ? -1 : 0;
		const unsigned int dac_mask = (channel_metadata == &fm->state->channels[5] && fm->state->dac_enabled) ? -1 : 0;

		short *sample_buffer_pointer;

		sample_buffer_pointer = sample_buffer;

		while (sample_buffer_pointer != sample_buffer_end)
		{
			/* The FM sample is 16-bit, so divide it so that it can be mixed with the other five FM channels and the PSG without clipping. */
			const int fm_sample = FM_Channel_GetSample(&channel) / VOLUME_DIVIDER;

			/* Do some boolean algebra to select the FM sample or the DAC sample. */
			const int sample = (fm_sample & ~dac_mask) | (fm->state->dac_sample & dac_mask);

			/* Output sample. */
			*sample_buffer_pointer++ += sample & left_mask;
			*sample_buffer_pointer++ += sample & right_mask;
		}
	}
}
