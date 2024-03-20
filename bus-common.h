#ifndef BUS_COMMON
#define BUS_COMMON

#include "clowncommon/clowncommon.h"

#include "clownmdemu.h"
#include "io-port.h"

typedef struct DataAndCallbacks
{
	const ClownMDEmu *data;
	const ClownMDEmu_Callbacks *frontend_callbacks;
} DataAndCallbacks;

typedef struct SyncState
{
	cc_u32f current_cycle;
} SyncState;

typedef struct SyncCPUState
{
	cc_u32f current_cycle;
	cc_u16f *cycle_countdown;
} SyncCPUState;

typedef struct CPUCallbackUserData
{
	DataAndCallbacks data_and_callbacks;
	struct
	{
		SyncCPUState m68k;
		SyncCPUState z80;
		SyncCPUState mcd_m68k;
		SyncState fm;
		SyncState psg;
		SyncState pcm;
	} sync;
} CPUCallbackUserData;

/* TODO: Move this to somewhere more specific. */
typedef struct IOPortToController_Parameters
{
	Controller *controller;
	const ClownMDEmu_Callbacks *frontend_callbacks;
} IOPortToController_Parameters;

typedef cc_u16f (*SyncCPUCommonCallback)(const ClownMDEmu *clownmdemu, void *user_data);

cc_u32f SyncCommon(SyncState *sync, cc_u32f target_cycle, cc_u32f clock_divisor);
void SyncCPUCommon(const ClownMDEmu *clownmdemu, SyncCPUState *sync, cc_u32f target_cycle, SyncCPUCommonCallback callback, const void *user_data);
cc_u8f SyncFM(CPUCallbackUserData *other_state, cc_u32f target_cycle);
void SyncPSG(CPUCallbackUserData *other_state, cc_u32f target_cycle);
void SyncPCM(CPUCallbackUserData *other_state, cc_u32f target_cycle);

#endif /* BUS_COMMON */
