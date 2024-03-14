#ifndef BUS_Z80_H
#define BUS_Z80_H

#include "bus-common.h"

void SyncZ80(const ClownMDEmu *clownmdemu, CPUCallbackUserData *other_state, cc_u32f target_cycle);
cc_u16f Z80ReadCallbackWithCycle(const void *user_data, cc_u16f address, cc_u32f target_cycle);
cc_u16f Z80ReadCallback(const void *user_data, cc_u16f address);
void Z80WriteCallbackWithCycle(const void *user_data, cc_u16f address, cc_u16f value, cc_u32f target_cycle);
void Z80WriteCallback(const void *user_data, cc_u16f address, cc_u16f value);

#endif /* BUS_Z80_H */
