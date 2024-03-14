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

typedef struct CPUCallbackUserData
{
	DataAndCallbacks data_and_callbacks;
	cc_u32f m68k_current_cycle;
	cc_u32f z80_current_cycle;
	cc_u32f mcd_m68k_current_cycle;
	cc_u32f fm_current_cycle;
	cc_u32f psg_current_cycle;
	cc_u32f pcm_current_cycle;
} CPUCallbackUserData;

/* TODO: Move this to somewhere more specific. */
typedef struct IOPortToController_Parameters
{
	Controller *controller;
	const ClownMDEmu_Callbacks *frontend_callbacks;
} IOPortToController_Parameters;

cc_u8f SyncFM(CPUCallbackUserData *other_state, cc_u32f target_cycle);
void SyncPSG(CPUCallbackUserData *other_state, cc_u32f target_cycle);
void SyncMCDPCM(CPUCallbackUserData *other_state, cc_u32f target_cycle);

#endif /* BUS_COMMON */
