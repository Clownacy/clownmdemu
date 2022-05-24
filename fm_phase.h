#ifndef FM_PHASE_H
#define FM_PHASE_H

typedef struct FM_Phase_State
{
	unsigned long position;
	unsigned long step;

	unsigned int f_number_and_block;
	unsigned int detune;
	unsigned int multiplier;
} FM_Phase_State;

void FM_Phase_State_Initialise(FM_Phase_State *phase);

void FM_Phase_SetFrequency(FM_Phase_State *phase, unsigned int f_number_and_block);
void FM_Phase_SetDetuneAndMultiplier(FM_Phase_State *phase, unsigned int detune, unsigned int multiplier);

unsigned int FM_Phase_Reset(FM_Phase_State *phase);
unsigned int FM_Phase_Increment(FM_Phase_State *phase);

#endif /* FM_PHASE_H */
