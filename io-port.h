#ifndef IO_PORT_H
#define IO_PORT_H

#include "clowncommon/clowncommon.h"

typedef cc_u8f (*IOPort_ReadCallback)(void *user_data, cc_u16f cycles);
typedef void (*IOPort_WriteCallback)(void *user_data, cc_u8f value, cc_u16f cycles);

typedef struct IOPort
{
	cc_u8f mask, cached_write;
	IOPort_ReadCallback read_callback;
	IOPort_WriteCallback write_callback;
} IOPort;

IOPort IOPort_Initialise(void);
void IOPort_SetCallbacks(IOPort *io_port, IOPort_ReadCallback read_callback, IOPort_WriteCallback write_callback);
#define IOPort_ReadControl(IO_PORT) (IO_PORT)->mask
#define IOPort_WriteControl(IO_PORT, MASK) (IO_PORT)->mask = (MASK)
cc_u8f IOPort_ReadData(IOPort *io_port, cc_u16f cycles, const void *user_data);
void IOPort_WriteData(IOPort *io_port, cc_u8f value, cc_u16f cycles, const void *user_data);

#endif /* IO_PORT_H */
