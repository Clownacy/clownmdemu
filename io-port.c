#include "io-port.h"

#include <string.h>

IOPort IOPort_Initialise(void)
{
	IOPort io_port;

	/* The standard Sega SDK bootcode uses this to detect soft-resets
	   (it checks if the control value is 0. */
	memset(&io_port, 0, sizeof(io_port));

	return io_port;
}

void IOPort_SetCallbacks(IOPort* const io_port, const IOPort_ReadCallback read_callback, const IOPort_WriteCallback write_callback)
{
	io_port->read_callback = read_callback;
	io_port->write_callback = write_callback;
}

cc_u8f IOPort_ReadData(IOPort* const io_port, const cc_u16f cycles, const void *user_data)
{
	if (io_port->read_callback == NULL)
		return 0;

	return (io_port->read_callback((void*)user_data, cycles) & ~io_port->mask) | io_port->cached_write;
}

void IOPort_WriteData(IOPort* const io_port, const cc_u8f value, const cc_u16f cycles, const void *user_data)
{
	if (io_port->write_callback == NULL)
		return;

	io_port->cached_write = value & io_port->mask; /* TODO: Is this really how the real thing does this? */
	io_port->write_callback((void*)user_data, io_port->cached_write, cycles);
}
