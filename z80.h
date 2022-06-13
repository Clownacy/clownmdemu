#ifndef Z80_H
#define Z80_H

#include "clowncommon.h"

typedef enum Z80_RegisterMode
{
	Z80_REGISTER_MODE_HL,
	Z80_REGISTER_MODE_IX,
	Z80_REGISTER_MODE_IY
} Z80_RegisterMode;

typedef struct Z80_Constant
{
	unsigned char parity_lookup[0x100];
} Z80_Constant;

typedef struct Z80_State
{
	Z80_RegisterMode register_mode;
	unsigned int cycles;
	unsigned short program_counter;
	unsigned short stack_pointer;
	unsigned char a, f, b, c, d, e, h, l;
	unsigned char a_, f_, b_, c_, d_, e_, h_, l_; /* Backup registers. */
	unsigned char ixh, ixl, iyh, iyl;
	unsigned char r, i;
	cc_bool interrupts_enabled;
} Z80_State;

typedef struct Z80_ReadAndWriteCallbacks
{
	unsigned int (*read)(void *user_data, unsigned int address);
	void (*write)(void *user_data, unsigned int address, unsigned int value);
	void *user_data;
} Z80_ReadAndWriteCallbacks;

typedef struct Z80
{
	const Z80_Constant *constant;
	Z80_State *state;
} Z80;

void Z80_Constant_Initialise(Z80_Constant *constant);
void Z80_State_Initialise(Z80_State *state);
void Z80_Reset(const Z80 *z80);
void Z80_Interrupt(const Z80 *z80, const Z80_ReadAndWriteCallbacks *callbacks);
void Z80_DoCycle(const Z80 *z80, const Z80_ReadAndWriteCallbacks *callbacks);

#endif /* Z80_H */
