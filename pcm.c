#include "pcm.h"

#include "clowncommon/clowncommon.h"

void PCM_State_Initialise(PCM_State *state)
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

void PCM_WriteRegister(const PCM *pcm, cc_u16f reg, cc_u8f value)
{
	PCM_ChannelState *current_channel = &pcm->state->channels[pcm->state->current_channel];
	size_t i;

	switch (reg)
	{
		case 0x00:
			current_channel->volume = value;
			break;
			
		case 0x01:
			current_channel->panning = value;
			break;
			
		case 0x02:
			current_channel->frequency &= 0xFF00;
			current_channel->frequency |= value;
			break;
			
		case 0x03:
			current_channel->frequency &= 0x00FF;
			current_channel->frequency |= value << 8;
			break;
			
		case 0x04:
			current_channel->loop_address &= 0xFF00;
			current_channel->loop_address |= value;
			break;
			
		case 0x05:
			current_channel->loop_address &= 0x00FF;
			current_channel->loop_address |= value << 8;
			break;

		case 0x06:
			current_channel->start_address = value;
			break;

		case 0x07:
			pcm->state->sounding = (value & 0x80) != 0;
			if (value & 0x40)
				pcm->state->current_channel = value & 7;
			else
				pcm->state->current_wave_bank = value & 0xF;
			break;

		case 0x08:
			for (i = 0; i < CC_COUNT_OF(pcm->state->channels); ++i)
			{
				pcm->state->channels[i].disabled = (value >> i) & 1;
				if (pcm->state->channels[i].disabled)
					pcm->state->channels[i].address = pcm->state->channels[i].start_address << 19;
			}
			break;
	}
}

cc_u8f PCM_ReadRegister(const PCM *pcm, cc_u16f reg)
{
	PCM_ChannelState *current_channel = &pcm->state->channels[pcm->state->current_channel];
	cc_u8f value = 0;
	size_t i;

	switch (reg)
	{
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
			for (i = 0; i < CC_COUNT_OF(pcm->state->channels); ++i)
				value |= pcm->state->channels[i].disabled << i;
			break;

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

void PCM_WriteWaveRAM(const PCM *pcm, cc_u16f address, cc_u8f value)
{
	pcm->state->wave_ram[(pcm->state->current_wave_bank << 12) + (address & 0xFFFF)] = value;
}

void PCM_Update(const PCM *pcm, cc_s16l *sample_buffer, size_t total_samples)
{
	cc_u8f wave_value;
	size_t i;
	size_t j;

	for (i = 0; i < CC_COUNT_OF(pcm->state->channels); ++i)
	{
		if (!pcm->state->channels[i].disabled)
		{
			for (j = 0; j < total_samples; ++j)
			{
				wave_value = pcm->state->wave_ram[(pcm->state->channels[i].address >> 11) & 0xFFFF];
				pcm->state->channels[i].address += pcm->state->channels[i].frequency;
				pcm->state->channels[i].address &= 0x7FFFFFF;

				if (wave_value == 0xFF)
				{
					pcm->state->channels[i].address = pcm->state->channels[i].loop_address << 11;
					wave_value = pcm->state->wave_ram[(pcm->state->channels[i].address >> 11) & 0xFFFF];
				}
			}
		}
	}
}