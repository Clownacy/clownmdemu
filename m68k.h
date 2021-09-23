#ifndef M68K_H
#define M68K_H

typedef struct M68k_State
{
	unsigned long data_registers[8];
	unsigned long address_registers[8];
	unsigned long program_counter;
	unsigned short status_register;

	unsigned char cycles_left_in_instruction;
} M68k_State;

typedef struct M68k_ReadWriteCallbacks
{
	unsigned char (*read_callback)(void *user_data, unsigned long address);
	void (*write_callback)(void *user_data, unsigned long address, unsigned char value);
	void *user_data;
} M68k_ReadWriteCallbacks;

void M68k_Reset(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks);
void M68k_Interrupt(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks, unsigned char level);
void M68k_DoCycle(M68k_State *state, const M68k_ReadWriteCallbacks *callbacks);

#endif /* M68K_H */
