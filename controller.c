#include "controller.h"

#include <string.h>

static void Controller_DoCycles(Controller* const controller, const cc_u16f cycles)
{
	if (controller->countdown >= cycles)
	{
		controller->countdown -= cycles;
	}
	else
	{
		controller->countdown = 0;
		controller->strobes = 0;
	}
}

static cc_bool Controller_GetButtonBit(Controller* const controller, const void *user_data, const Controller_Button button)
{
	return !controller->callback((void*)user_data, button);
}

void Controller_Initialise(Controller* const controller, const Controller_Callback callback)
{
	memset(controller, 0, sizeof(*controller));

	controller->callback = callback;
}

cc_u8f Controller_Read(Controller* const controller, const cc_u16f cycles, const void *user_data)
{
	Controller_DoCycles(controller, cycles);

	if (controller->th_bit)
	{
		switch (controller->strobes)
		{
			default:
				return (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_C) << 5)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_B) << 4)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_RIGHT) << 3)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_LEFT) << 2)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_DOWN) << 1)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_UP) << 0);

			case 3:
				return (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_C) << 5)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_B) << 4)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_MODE) << 3)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_X) << 2)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_Y) << 1)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_Z) << 0);
		}
	}
	else
	{
		switch (controller->strobes)
		{
			default:
				return (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_START) << 5)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_A) << 4)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_DOWN) << 1)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_UP) << 0);

			case 2:
				return (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_START) << 5)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_A) << 4);

			case 3:
				return (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_START) << 5)
				     | (Controller_GetButtonBit(controller, user_data, CONTROLLER_BUTTON_A) << 4)
				     | 0xF;
		}
	}
}

void Controller_Write(Controller* const controller, const cc_u8f value, const cc_u16f cycles)
{
	const cc_bool new_th_bit = (value & 0x40) != 0;

	Controller_DoCycles(controller, cycles);

	if (new_th_bit && !controller->th_bit)
	{
		controller->strobes = (controller->strobes + 1) % 4;
		controller->countdown = 1500; /* 1.5ms */
	}

	controller->th_bit = new_th_bit;
}
