#include "bus-common.h"

#include <assert.h>

#include "fm.h"
#include "pcm.h"
#include "psg.h"

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
	/* Store this in a local variables to make the upcoming code faster. */
	cc_u16f countdown = *sync->cycle_countdown;

	while (sync->current_cycle < target_cycle)
	{
		const cc_u32f cycles_to_do = CC_MIN(countdown, target_cycle - sync->current_cycle);

		assert(target_cycle >= sync->current_cycle); /* If this fails, then we must have failed to synchronise somewhere! */

		countdown -= cycles_to_do;

		if (countdown == 0)
			countdown = callback(clownmdemu, (void*)user_data);

		sync->current_cycle += cycles_to_do;
	}

	/* Store this back in memory for later. */
	*sync->cycle_countdown = countdown;
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

cc_u8f SyncFM(CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	return FM_Update(&other_state->data_and_callbacks.data->fm, SyncCommon(&other_state->sync.fm, target_cycle, CLOWNMDEMU_M68K_CLOCK_DIVIDER), GenerateFMAudio, other_state);
}

static void GeneratePSGAudio(const ClownMDEmu* const clownmdemu, cc_s16l* const sample_buffer, const size_t total_frames)
{
	PSG_Update(&clownmdemu->psg, sample_buffer, total_frames);
}

void SyncPSG(CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	const cc_u32f frames_to_generate = SyncCommon(&other_state->sync.psg, target_cycle, CLOWNMDEMU_Z80_CLOCK_DIVIDER * CLOWNMDEMU_PSG_SAMPLE_RATE_DIVIDER);

	/* TODO: Is this check necessary? */
	if (frames_to_generate != 0)
		other_state->data_and_callbacks.frontend_callbacks->psg_audio_to_be_generated((void*)other_state->data_and_callbacks.frontend_callbacks->user_data, frames_to_generate, GeneratePSGAudio);
}

static void GeneratePCMAudio(const ClownMDEmu* const clownmdemu, cc_s16l* const sample_buffer, const size_t total_frames)
{
	PCM_Update(&clownmdemu->pcm, sample_buffer, total_frames);
}

void SyncPCM(CPUCallbackUserData* const other_state, const cc_u32f target_cycle)
{
	other_state->data_and_callbacks.frontend_callbacks->pcm_audio_to_be_generated((void*)other_state->data_and_callbacks.frontend_callbacks->user_data, SyncCommon(&other_state->sync.pcm, target_cycle, CLOWNMDEMU_MCD_M68K_CLOCK_DIVIDER * CLOWNMDEMU_PCM_SAMPLE_RATE_DIVIDER), GeneratePCMAudio);
}
