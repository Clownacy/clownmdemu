#ifndef FM_PHASE_H
#define FM_PHASE_H

typedef struct FM_Phase
{
	unsigned long position;
	unsigned long step;

	unsigned int f_number_and_block;
	unsigned int detune;
	unsigned int multiplier;
} FM_Phase;

void FM_Phase_Initialise(FM_Phase *phase);
void FM_Phase_SetFrequency(FM_Phase *phase, unsigned int f_number_and_block);
void FM_Phase_SetDetuneAndMultiplier(FM_Phase *phase, unsigned int detune_and_multiplier);
unsigned int FM_Phase_Increment(FM_Phase *phase);

#endif /* FM_PHASE_H */
