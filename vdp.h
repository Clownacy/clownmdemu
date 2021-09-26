#ifndef VDP_H
#define VDP_H

typedef struct VDP_State
{
	
} VDP_State;

unsigned short VDP_ReadStatus(VDP_State *state);
void VDP_WriteData(VDP_State *state, unsigned short value);
void VDP_WriteControl(VDP_State *state, unsigned short value);

#endif /* VDP_H */
