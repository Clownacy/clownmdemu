#ifndef FM_OPERATOR_H
#define FM_OPERATOR_H

typedef struct FM_Operator_Constant
{
	unsigned short logarithmic_attenuation_sine_table[0x100];
	unsigned short power_table[0x100];
} FM_Operator_Constant;

void FM_Operator_Constant_Initialise(FM_Operator_Constant *constant);
int FM_Operator_Process(const FM_Operator_Constant *constant, unsigned int phase, int phase_modulation, unsigned int attenuation);

#endif /* FM_OPERATOR_H */
