#include "bus-common.h"

#include <assert.h>

#include "fm.h"
#include "pcm.h"
#include "psg.h"

static void FMCallbackWrapper(const ClownMDEmu* const clownmdemu, cc_s16l* const sample_buffer, const size_t total_frames)
{
	FM_OutputSamples(&clownmdemu->fm, sample_buffer, total_frames);
}

static void GenerateFMAudio(const void* const user_data, const cc_u32f total_frames)
{
	CPUCallbackUserData* const callback_user_data = (CPUCallbackUserData*)user_data;

	callback_user_data->data_and_callbacks.frontend_callbacks->fm_audio_to_be_generated((void*)callback_user_data->data_and_callbacks.frontend_callbacks->user_data, total_frames, FMCallbackWrapper);
}

cc_u8f SyncFM(CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	const cc_u32f fm_target_cycle = target_cycle / CLOWNMDEMU_M68K_CLOCK_DIVIDER;

	const cc_u32f cycles_to_do = fm_target_cycle - other_state->fm_current_cycle;

	assert(fm_target_cycle >= other_state->fm_current_cycle); /* If this fails, then we must have failed to synchronise somewhere! */

	other_state->fm_current_cycle = fm_target_cycle;

	return FM_Update(&other_state->data_and_callbacks.data->fm, cycles_to_do, GenerateFMAudio, other_state);
}

static void GeneratePSGAudio(const ClownMDEmu* const clownmdemu, cc_s16l* const sample_buffer, const size_t total_samples)
{
	PSG_Update(&clownmdemu->psg, sample_buffer, total_samples);
}

void SyncPSG(CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	const cc_u32f psg_target_cycle = target_cycle / (CLOWNMDEMU_Z80_CLOCK_DIVIDER * CLOWNMDEMU_PSG_SAMPLE_RATE_DIVIDER);

	const size_t samples_to_generate = psg_target_cycle - other_state->psg_current_cycle;

	assert(psg_target_cycle >= other_state->psg_current_cycle); /* If this fails, then we must have failed to synchronise somewhere! */

	if (samples_to_generate != 0)
	{
		other_state->data_and_callbacks.frontend_callbacks->psg_audio_to_be_generated((void*)other_state->data_and_callbacks.frontend_callbacks->user_data, samples_to_generate, GeneratePSGAudio);

		other_state->psg_current_cycle = psg_target_cycle;
	}
}

static void GeneratePCMAudio(const ClownMDEmu* const clownmdemu, cc_s16l* const sample_buffer, const size_t total_samples)
{
	PCM_Update(&clownmdemu->pcm, sample_buffer, total_samples);
}

void SyncPCM(CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	const cc_u32f pcm_target_cycle = target_cycle / CLOWNMDEMU_MCD_M68K_CLOCK_DIVIDER;

	const size_t samples_to_generate = pcm_target_cycle - other_state->pcm_current_cycle;

	assert(pcm_target_cycle >= other_state->pcm_current_cycle); /* If this fails, then we must have failed to synchronise somewhere! */

	if (samples_to_generate != 0)
	{
		other_state->data_and_callbacks.frontend_callbacks->pcm_audio_to_be_generated((void*)other_state->data_and_callbacks.frontend_callbacks->user_data, samples_to_generate, GeneratePCMAudio);

		other_state->pcm_current_cycle = pcm_target_cycle;
	}
}
