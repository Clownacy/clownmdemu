#ifndef CLOWNMDEMU_H
#define CLOWNMDEMU_H

#include <stddef.h>

/* TODO - Documentation */
/* TODO - Library linkage stuff */

void ClownMDEmu_Init(void *state);
void ClownMDEmu_Deinit(void *state);
void ClownMDEmu_Iterate(void *state, void (*video_callback)(void *pixels, size_t width, size_t height));
void ClownMDEmu_UpdateROM(void *state, const unsigned char *rom_buffer, size_t rom_size);
void ClownMDEmu_SetROMWriteable(void *state, unsigned char rom_writeable);
void ClownMDEmu_Reset(void *state_void);
void ClownMDEmu_SetPAL(void *state_void, unsigned char pal);
void ClownMDEmu_SetJapanese(void *state_void, unsigned char japanese);
size_t ClownMDEmu_GetStateSize(void);

#endif /* CLOWNMDEMU_H */
