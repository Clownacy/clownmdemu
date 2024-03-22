#include "bus-common.h"

#include <assert.h>
#include <string.h>

#include "fm.h"
#include "pcm.h"
#include "psg.h"

CycleMegaDrive MakeCycleMegaDrive(const cc_u32f cycle)
{
	CycleMegaDrive cycle_mega_drive;
	cycle_mega_drive.cycle = cycle;
	return cycle_mega_drive;
}

CycleMegaCD MakeCycleMegaCD(const cc_u32f cycle)
{
	CycleMegaCD cycle_mega_cd;
	cycle_mega_cd.cycle = cycle;
	return cycle_mega_cd;
}

static cc_u32f ConvertCycle(const cc_u32f cycle, const cc_u32f* const scale_halves)
{
	const cc_u32f cycle_upper = cycle >> 16;
	const cc_u32f cycle_lower = cycle & 0xFFFF;

	const cc_u32f result_upper = cycle_upper * scale_halves[0];
	const cc_u32f result_lower1 = cycle_upper * scale_halves[1];
	const cc_u32f result_lower2 = cycle_lower * scale_halves[0];

	const cc_u32f result = (result_upper << 1) + (result_lower1 >> 15) + (result_lower2 >> 15);

	assert(cycle <= 0xFFFFFFFF);

	return result;
}

CycleMegaCD CycleMegaDriveToMegaCD(const ClownMDEmu* const clownmdemu, const CycleMegaDrive cycle)
{
	/* These are 32-bit integers split in half. */
	const cc_u32f ntsc[2] = {0x7732, 0x1ECA}; /* 0x80000000 * CLOWNMDEMU_MCD_MASTER_CLOCK / CLOWNMDEMU_MASTER_CLOCK_NTSC */
	const cc_u32f pal[2] = {0x784B, 0x02AF}; /* 0x80000000 * CLOWNMDEMU_MCD_MASTER_CLOCK / CLOWNMDEMU_MASTER_CLOCK_PAL */

	CycleMegaCD new_cycle;
	new_cycle.cycle = ConvertCycle(cycle.cycle, clownmdemu->configuration->general.tv_standard == CLOWNMDEMU_TV_STANDARD_NTSC ? ntsc : pal);
	return new_cycle;
}

CycleMegaDrive CycleMegaCDToMegaDrive(const ClownMDEmu* const clownmdemu, const CycleMegaCD cycle)
{
	/* These are 32-bit integers split in half. */
	const cc_u32f ntsc[2] = {0x8974, 0x5BF2}; /* 0x80000000 * CLOWNMDEMU_MASTER_CLOCK_NTSC / CLOWNMDEMU_MCD_MASTER_CLOCK */
	const cc_u32f pal[2] = {0x8833, 0x655D}; /* 0x80000000 * CLOWNMDEMU_MASTER_CLOCK_PAL / CLOWNMDEMU_MCD_MASTER_CLOCK */

	CycleMegaDrive new_cycle;
	new_cycle.cycle = ConvertCycle(cycle.cycle, clownmdemu->configuration->general.tv_standard == CLOWNMDEMU_TV_STANDARD_NTSC ? ntsc : pal);
	return new_cycle;
}

cc_u32f SyncCommon(SyncState* const sync, const cc_u32f target_cycle, const cc_u32f clock_divisor)
{
	const cc_u32f native_target_cycle = target_cycle / clock_divisor;

	const cc_u32f cycles_to_do = native_target_cycle - sync->current_cycle;

	assert(native_target_cycle >= sync->current_cycle); /* If this fails, then we must have failed to synchronise somewhere! */

	sync->current_cycle = native_target_cycle;

	return cycles_to_do;
}

void SyncCPUCommon(const ClownMDEmu* const clownmdemu, SyncCPUState* const sync, const cc_u32f target_cycle, const SyncCPUCommonCallback callback, const void* const user_data)
{
	/* Store this in a local variable to make the upcoming code faster. */
	cc_u16f countdown = *sync->cycle_countdown;

	if (countdown == 0)
	{
		sync->current_cycle = target_cycle;
	}
	else
	{
		while (sync->current_cycle < target_cycle)
		{
			const cc_u32f cycles_to_do = CC_MIN(countdown, target_cycle - sync->current_cycle);

			sync->current_cycle += cycles_to_do;

			countdown -= cycles_to_do;

			if (countdown == 0)
				countdown = callback(clownmdemu, (void*)user_data);
		}

		/* Store this back in memory for later. */
		*sync->cycle_countdown = countdown;
	}
}

static void FMCallbackWrapper(const ClownMDEmu* const clownmdemu, cc_s16l* const sample_buffer, const size_t total_frames)
{
	FM_OutputSamples(&clownmdemu->fm, sample_buffer, total_frames);
}

static void GenerateFMAudio(const void* const user_data, const cc_u32f total_frames)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	callback_user_data->data_and_callbacks.frontend_callbacks->fm_audio_to_be_generated((void*)callback_user_data->data_and_callbacks.frontend_callbacks->user_data, total_frames, FMCallbackWrapper);
}

cc_u8f SyncFM(CPUCallbackUserData* const other_state, const CycleMegaDrive target_cycle)
{
	return FM_Update(&other_state->data_and_callbacks.data->fm, SyncCommon(&other_state->sync.fm, target_cycle.cycle, CLOWNMDEMU_M68K_CLOCK_DIVIDER), GenerateFMAudio, other_state);
}

static void GeneratePSGAudio(const ClownMDEmu* const clownmdemu, cc_s16l* const sample_buffer, const size_t total_frames)
{
	PSG_Update(&clownmdemu->psg, sample_buffer, total_frames);
}

void SyncPSG(CPUCallbackUserData* const other_state, const CycleMegaDrive target_cycle)
{
	const cc_u32f frames_to_generate = SyncCommon(&other_state->sync.psg, target_cycle.cycle, CLOWNMDEMU_Z80_CLOCK_DIVIDER * CLOWNMDEMU_PSG_SAMPLE_RATE_DIVIDER);

	/* TODO: Is this check necessary? */
	if (frames_to_generate != 0)
		other_state->data_and_callbacks.frontend_callbacks->psg_audio_to_be_generated((void*)other_state->data_and_callbacks.frontend_callbacks->user_data, frames_to_generate, GeneratePSGAudio);
}

static void GeneratePCMAudio(const ClownMDEmu* const clownmdemu, cc_s16l* const sample_buffer, const size_t total_frames)
{
	PCM_Update(&clownmdemu->pcm, sample_buffer, total_frames);
}

void SyncPCM(CPUCallbackUserData* const other_state, const CycleMegaCD target_cycle)
{
	other_state->data_and_callbacks.frontend_callbacks->pcm_audio_to_be_generated((void*)other_state->data_and_callbacks.frontend_callbacks->user_data, SyncCommon(&other_state->sync.pcm, target_cycle.cycle, CLOWNMDEMU_MCD_M68K_CLOCK_DIVIDER * CLOWNMDEMU_PCM_SAMPLE_RATE_DIVIDER), GeneratePCMAudio);
}

static void GenerateCDDAAudio(const ClownMDEmu* const clownmdemu, cc_s16l* const sample_buffer, const size_t total_frames)
{
	const cc_u32f total_channels = 2;

	cc_u32f total_frames_done = 0;

	if (clownmdemu->state->mega_cd.cdda.playing)
	{
		/* Loop reading samples until we reach the end of either the input data or the output buffer. */
		for (;;)
		{
			const cc_u32f frames_done = clownmdemu->callbacks.cd_audio_read((void*)clownmdemu->callbacks.user_data, sample_buffer + total_frames_done * total_channels, total_frames - total_frames_done);

			total_frames_done = frames_done;

			if (frames_done == 0 || total_frames_done == total_frames)
				break;

			if (clownmdemu->state->mega_cd.cdda.repeating)
			{
				clownmdemu->callbacks.cd_track_seeked((void*)clownmdemu->callbacks.user_data, clownmdemu->state->mega_cd.cdda.current_track);
			}
			else
			{
				clownmdemu->state->mega_cd.cdda.playing = cc_false;
				break;
			}
		}
	}

	/* Clear any samples that we could not read from the disc. */
	memset(sample_buffer + total_frames_done * total_channels, 0, (total_frames - total_frames_done) * sizeof(cc_s16l) * total_channels);
}

void SyncCDDA(CPUCallbackUserData* const other_state, const cc_u32f total_frames)
{
	other_state->data_and_callbacks.frontend_callbacks->cdda_audio_to_be_generated((void*)other_state->data_and_callbacks.frontend_callbacks->user_data, total_frames, GenerateCDDAAudio);
}
