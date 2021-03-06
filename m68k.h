#ifndef M68K_H
#define M68K_H

#include "clowncommon.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct M68k_State
{
	unsigned long data_registers[8];
	unsigned long address_registers[8];
	unsigned long user_stack_pointer;
	unsigned long program_counter;
	unsigned short status_register;
	unsigned short instruction_register;
	cc_bool halted;

	unsigned char cycles_left_in_instruction;
} M68k_State;

typedef struct M68k_ReadWriteCallbacks
{
	unsigned int (*read_callback)(const void *user_data, unsigned long address, cc_bool do_high_byte, cc_bool do_low_byte);
	void (*write_callback)(const void *user_data, unsigned long address, cc_bool do_high_byte, cc_bool do_low_byte, unsigned int value);
	const void *user_data;
} M68k_ReadWriteCallbacks;

void M68k_Reset(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks);
void M68k_Interrupt(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, unsigned int level);
void M68k_DoCycle(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks);

#ifdef __cplusplus
}
#endif

#endif /* M68K_H */
