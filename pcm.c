#include "pcm.h"

#include "clowncommon/clowncommon.h"

void PCM_State_Initialise(PCM_State* const state)
{
	size_t i;

	for (i = 0; i < CC_COUNT_OF(state->channels); ++i)
	{
		state->channels[i].disabled = cc_true;
		state->channels[i].volume = 0;
		state->channels[i].panning = 0;
		state->channels[i].frequency = 0;
		state->channels[i].loop_address = 0;
		state->channels[i].start_address = 0;
		state->channels[i].address = 0;
	}

	state->sounding = cc_false;
	state->current_wave_bank = 0;
	state->current_channel = 0;
}

void PCM_WriteRegister(const PCM* const pcm, const cc_u16f reg, const cc_u8f value)
{
	PCM_ChannelState* const current_channel = &pcm->state->channels[pcm->state->current_channel];

	switch (reg)
	{
		case 0:
			current_channel->volume = value;
			break;
			
		case 1:
			current_channel->panning = value;
			break;
			
		case 2:
			current_channel->frequency &= 0xFF00;
			current_channel->frequency |= value;
			break;
			
		case 3:
			current_channel->frequency &= 0x00FF;
			current_channel->frequency |= (cc_u16f)value << 8;
			break;
			
		case 4:
			current_channel->loop_address &= 0xFF00;
			current_channel->loop_address |= value;
			break;
			
		case 5:
			current_channel->loop_address &= 0x00FF;
			current_channel->loop_address |= (cc_u16f)value << 8;
			break;

		case 6:
			current_channel->start_address = value;
			break;

		case 7:
			pcm->state->sounding = (value & 0x80) != 0;

			if ((value & 0x40) != 0)
				pcm->state->current_channel = value & 7;
			else
				pcm->state->current_wave_bank = value & 0xF;

			break;

		case 8:
		{
			size_t i;

			for (i = 0; i < CC_COUNT_OF(pcm->state->channels); ++i)
				pcm->state->channels[i].disabled = ((value >> i) & 1) != 0;

			break;
		}
	}
}

cc_u8f PCM_ReadRegister(const PCM* const pcm, const cc_u16f reg)
{
	cc_u8f value;

	PCM_ChannelState* const current_channel = &pcm->state->channels[pcm->state->current_channel];

	value = 0;

	switch (reg)
	{
		size_t i;

		case 0x00:
			value = current_channel->volume;
			break;
			
		case 0x01:
			value = current_channel->panning;
			break;
			
		case 0x02:
			value = current_channel->frequency & 0xFF;
			break;
			
		case 0x03:
			value = (current_channel->frequency >> 8) & 0xFF;
			break;
			
		case 0x04:
			value = current_channel->loop_address & 0xFF;
			break;
			
		case 0x05:
			value = (current_channel->loop_address >> 8) & 0xFF;
			break;

		case 0x06:
			value = current_channel->start_address;
			break;

		case 0x08:
		{
			size_t i;

			for (i = 0; i < CC_COUNT_OF(pcm->state->channels); ++i)
				value |= pcm->state->channels[i].disabled << i;

			break;
		}

		case 0x10:
		case 0x12:
		case 0x14:
		case 0x16:
		case 0x18:
		case 0x1A:
		case 0x1C:
		case 0x1E:
			value = (pcm->state->channels[(reg - 0x10) / 2].address >> 11) & 0xFF;
			break;

		case 0x11:
		case 0x13:
		case 0x15:
		case 0x17:
		case 0x19:
		case 0x1B:
		case 0x1D:
		case 0x1F:
			value = (pcm->state->channels[(reg - 0x11) / 2].address >> 19) & 0xFF;
			break;
	}

	return value;
}

void PCM_WriteWaveRAM(const PCM* const pcm, const cc_u16f address, const cc_u8f value)
{
	pcm->state->wave_ram[(pcm->state->current_wave_bank << 12) + (address & 0xFFF)] = value;
}

void PCM_Update(const PCM* const pcm, cc_s16l* const sample_buffer, const size_t total_frames)
{
	/* TODO: Sounding (ew). */
	/* TODO: Invert these loops - that should be more cache-efficient. */
	size_t i;

	for (i = 0; i < CC_COUNT_OF(pcm->state->channels); ++i)
	{
		if (!pcm->state->channels[i].disabled && pcm->state->sounding)
		{
			cc_s16l *sample_pointer;
			size_t j;

			sample_pointer = sample_buffer;

			for (j = 0; j < total_frames; ++j)
			{
				cc_s16f wave_value;

				/* Read sample and advance address. */
				wave_value = pcm->state->wave_ram[(pcm->state->channels[i].address >> 11) & 0xFFFF];
				pcm->state->channels[i].address += pcm->state->channels[i].frequency;
				pcm->state->channels[i].address &= 0x7FFFFFF;

				/* Handle looping. */
				if (wave_value == 0xFF)
				{
					pcm->state->channels[i].address = pcm->state->channels[i].loop_address << 11;
					wave_value = pcm->state->wave_ram[(pcm->state->channels[i].address >> 11) & 0xFFFF];
				}

				/* Convert from sign-magnitude to two's-complement. */
				if ((wave_value & 0x80) == 0)
					wave_value = -wave_value;
				else if (wave_value != 0xFF)
					wave_value &= 0x7F;
				else
					wave_value = 0;

				/* Apply volume and panning */
				/* TODO: Maybe try to find a better way to balance the output volume? */
				*sample_pointer++ += (wave_value * pcm->state->channels[i].volume * (pcm->state->channels[i].panning & 0xF) * 0x1000) / 0x7698F;
				*sample_pointer++ += (wave_value * pcm->state->channels[i].volume * ((pcm->state->channels[i].panning >> 4) & 0xF) * 0x1000) / 0x7698F;
			}
		}
		else
		{
			pcm->state->channels[i].address = (cc_u32f)pcm->state->channels[i].start_address << 19;
		}
	}
}
