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

#include "fm.h"

#include <math.h>

#include "clowncommon/clowncommon.h"

#include "error.h"

void FM_Constant_Initialise(FM_Constant *constant)
{
	FM_Channel_Constant_Initialise(&constant->channels);
}

void FM_State_Initialise(FM_State *state)
{
	FM_ChannelMetadata *channel;
	cc_u8f i;

	for (channel = &state->channels[0]; channel < &state->channels[CC_COUNT_OF(state->channels)]; ++channel)
	{
		FM_Channel_State_Initialise(&channel->state);

		channel->cached_upper_frequency_bits = 0;

		/* Panning must be enabled by default. Without this, Sonic 1's 'Sega' chant doesn't play. */
		channel->pan_left = cc_true;
		channel->pan_right = cc_true;
	}

	state->port = 0 * 3;
	state->address = 0;

	state->dac_sample = 0;
	state->dac_enabled = cc_false;

	state->leftover_cycles = 0;
	state->raw_timer_a_value = 0;

	for (i = 0; i < 2; ++i)
	{
		state->timers[i].value = 0;
		state->timers[i].counter = 0;
		state->timers[i].enabled = cc_false;
	}

	state->status = 0;
}

void FM_Parameters_Initialise(FM *fm, const FM_Configuration *configuration, const FM_Constant *constant, FM_State *state)
{
	cc_u16f i;

	fm->configuration = configuration;
	fm->constant = constant;
	fm->state = state;

	for (i = 0; i < CC_COUNT_OF(fm->channels); ++i)
		FM_Channel_Parameters_Initialise(&fm->channels[i], &constant->channels, &state->channels[i].state);
}

void FM_DoAddress(const FM *fm, cc_u8f port, cc_u8f address)
{
	fm->state->port = port * 3;
	fm->state->address = address;
}

void FM_DoData(const FM *fm, cc_u8f data)
{
	/* Set BUSY flag. */
	fm->state->status |= 0x80;
	/* The YM2612's BUSY flag is always active for exactly 32 internal cycles.
	   If I remember correctly, the YM3438 actually gives the BUSY flag
	   different durations based on the pending operation. */
	/* TODO: YM3438 BUSY flag durations. */
	fm->state->busy_flag_counter = 32 * 6;

	if (fm->state->address < 0x30)
	{
		if (fm->state->port == 0)
		{
			switch (fm->state->address)
			{
				default:
					PrintError("Unrecognised FM address latched (0x%02" CC_PRIXFAST16 ")", fm->state->address);
					break;

				case 0x22:
					/* TODO: LFO. */
					break;

				case 0x24:
					/* Oddly, the YM2608 manual describes these timers being twice as fast as they are here. */
					fm->state->raw_timer_a_value &= 3;
					fm->state->raw_timer_a_value |= data << 2;
					/* TODO: According to http://md.railgun.works/index.php?title=YM2612, this doesn't happen
					   here: address 0x25 must be written to in order to update the timer proper. */
					fm->state->timers[0].value = FM_SAMPLE_RATE_DIVIDER * (0x400 - fm->state->raw_timer_a_value);
					break;

				case 0x25:
					fm->state->raw_timer_a_value &= ~3;
					fm->state->raw_timer_a_value |= data & 3;
					fm->state->timers[0].value = FM_SAMPLE_RATE_DIVIDER * (0x400 - fm->state->raw_timer_a_value);
					break;

				case 0x26:
					fm->state->timers[1].value = FM_SAMPLE_RATE_DIVIDER * 16 * (0x100 - data);
					break;

				case 0x27:
				{
					/* TODO: FM3 special mode. */

					cc_u8f i;

					for (i = 0; i < CC_COUNT_OF(fm->state->timers); ++i)
					{
						/* Only reload the timer on a rising edge. */
						if ((data & (1 << (0 + i))) != 0 && (fm->state->cached_address_27 & (1 << (0 + i))) == 0)
							fm->state->timers[i].counter = fm->state->timers[i].value;

						/* Enable the timer. */
						fm->state->timers[i].enabled = (data & (1 << (2 + i))) != 0;

						/* Clear the 'timer expired' flag. */
						if ((data & (1 << (4 + i))) != 0)
							fm->state->status &= ~(1 << i);
					}

					/* Cache the contents of this write for the above rising-edge detection. */
					fm->state->cached_address_27 = data;

					break;
				}

				case 0x28:
				{
					/* Key on/off. */
					/* There's a gap between channels 3 and 4. */
					/* TODO - Check what happens if you try to access the 'gap' channels on real hardware. */
					static const cc_u16f table[] = {0, 1, 2, 0, 3, 4, 5, 0};
					const FM_Channel* const channel = &fm->channels[table[data & 7]];

					FM_Channel_SetKeyOn(channel, 0, (data & (1 << 4)) != 0);
					FM_Channel_SetKeyOn(channel, 2, (data & (1 << 5)) != 0);
					FM_Channel_SetKeyOn(channel, 1, (data & (1 << 6)) != 0);
					FM_Channel_SetKeyOn(channel, 3, (data & (1 << 7)) != 0);

					break;
				}

				case 0x2A:
					/* DAC sample. */
					/* Convert from unsigned 8-bit PCM to signed 16-bit PCM. */
					fm->state->dac_sample = ((cc_s16f)data - 0x80) * (0x100 / FM_VOLUME_DIVIDER);
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
		const cc_u16f channel_index = fm->state->address & 3;
		FM_ChannelMetadata* const channel_metadata = &fm->state->channels[fm->state->port + channel_index];
		const FM_Channel* const channel = &fm->channels[fm->state->port + channel_index];

		/* There is no fourth channel per slot. */
		/* TODO: See how real hardware handles this. */
		if (channel_index != 3)
		{
			if (fm->state->address < 0xA0)
			{
				/* Per-operator. */
				const cc_u16f operator_index = (fm->state->address >> 2) & 3;

				switch (fm->state->address / 0x10)
				{
					default:
						PrintError("Unrecognised FM address latched (0x%02" CC_PRIXFAST16 ")", fm->state->address);
						break;

					case 0x30 / 0x10:
						/* Detune and multiplier. */
						FM_Channel_SetDetuneAndMultiplier(channel, operator_index, (data >> 4) & 7, data & 0xF);
						break;

					case 0x40 / 0x10:
						/* Total level. */
						FM_Channel_SetTotalLevel(channel, operator_index, data & 0x7F);
						break;

					case 0x50 / 0x10:
						/* Key scale and attack rate. */
						FM_Channel_SetKeyScaleAndAttackRate(channel, operator_index, (data >> 6) & 3, data & 0x1F);
						break;

					case 0x60 / 0x10:
						/* Amplitude modulation on and decay rate. */
						/* TODO: LFO. */
						FM_Channel_DecayRate(channel, operator_index, data & 0x1F);
						break;

					case 0x70 / 0x10:
						/* Sustain rate. */
						FM_Channel_SetSustainRate(channel, operator_index, data & 0x1F);
						break;

					case 0x80 / 0x10:
						/* Sustain level and release rate. */
						FM_Channel_SetSustainLevelAndReleaseRate(channel, operator_index, (data >> 4) & 0xF, data & 0xF);
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
						PrintError("Unrecognised FM address latched (0x%02" CC_PRIXFAST16 ")", fm->state->address);
						break;

					case 0xA8 / 4:
					case 0xAC / 4:
						break;

					case 0xA0 / 4:
						/* Frequency low bits. */
						FM_Channel_SetFrequency(channel, data | (channel_metadata->cached_upper_frequency_bits << 8));
						break;

					case 0xA4 / 4:
						/* Frequency high bits. */
						/* http://gendev.spritesmind.net/forum/viewtopic.php?p=5621#p5621 */
						channel_metadata->cached_upper_frequency_bits = data & 0x3F;
						break;

					case 0xB0 / 4:
						FM_Channel_SetFeedbackAndAlgorithm(channel, (data >> 3) & 7, data & 7);
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

void FM_OutputSamples(const FM *fm, cc_s16l *sample_buffer, cc_u32f total_frames)
{
	const cc_s16l* const sample_buffer_end = &sample_buffer[total_frames * 2];

	cc_u16f i;

	for (i = 0; i < CC_COUNT_OF(fm->state->channels); ++i)
	{
		if (!fm->configuration->channel_disabled[i])
		{
			FM_ChannelMetadata *channel_metadata = &fm->state->channels[i];
			const FM_Channel* const channel = &fm->channels[i];

			/* These will be used in later boolean algebra, to avoid branches. */
			const cc_bool left_enabled = channel_metadata->pan_left;
			const cc_bool right_enabled = channel_metadata->pan_right;
			const cc_bool fm_enabled = channel_metadata != &fm->state->channels[5] || !fm->state->dac_enabled;

			const cc_s16f dac_sample = !fm_enabled ? fm->state->dac_sample : 0;

			cc_s16l *sample_buffer_pointer;

			sample_buffer_pointer = sample_buffer;

			while (sample_buffer_pointer != sample_buffer_end)
			{
				/* The FM sample is 14-bit, so convert it to 16-bit and then divide it so that it
				   can be mixed with the other five FM channels and the PSG without clipping. */
				const cc_s16f fm_sample = FM_Channel_GetSample(channel) * 4 / FM_VOLUME_DIVIDER;

				/* Do some boolean algebra to select the FM sample or the DAC sample. */
				const cc_s16f sample = (fm_enabled ? fm_sample : 0) | dac_sample;

				/* Apply panning. */
				const cc_s16f left_sample = left_enabled ? sample : 0;
				const cc_s16f right_sample = right_enabled ? sample : 0;

				/* Output sample. */
				*sample_buffer_pointer++ += left_sample;
				*sample_buffer_pointer++ += right_sample;
			}
		}
	}
}

cc_u8f FM_Update(const FM *fm, cc_u32f cycles_to_do, void (*fm_audio_to_be_generated)(const void *user_data, cc_u32f total_frames), const void *user_data)
{
	cc_u8f i;

	const cc_u32f total_frames = (fm->state->leftover_cycles + cycles_to_do) / FM_SAMPLE_RATE_DIVIDER;

	fm->state->leftover_cycles = (fm->state->leftover_cycles + cycles_to_do) % FM_SAMPLE_RATE_DIVIDER;

	if (total_frames != 0)
		fm_audio_to_be_generated(user_data, total_frames);

	/* Decrement the timers. */
	for (i = 0; i < CC_COUNT_OF(fm->state->timers); ++i)
	{
		if (fm->state->timers[i].counter != 0)
		{
			fm->state->timers[i].counter -= CC_MIN(fm->state->timers[i].counter, cycles_to_do);

			if (fm->state->timers[i].counter == 0)
			{
				/* Set the 'timer expired' flag. */
				fm->state->status |= fm->state->timers[i].enabled ? 1 << i : 0;

				/* Reload the timer's counter. */
				fm->state->timers[i].counter = fm->state->timers[i].value;
			}
		}
	}

	/* Decrement the BUSY flag counter. */
	if (fm->state->busy_flag_counter != 0)
	{
		fm->state->busy_flag_counter -= CC_MIN(fm->state->busy_flag_counter, cycles_to_do);

		/* Clear BUSY flag if the counter has elapsed. */
		if (fm->state->busy_flag_counter == 0)
			fm->state->status &= ~0x80;
	}

	return fm->state->status;
}
