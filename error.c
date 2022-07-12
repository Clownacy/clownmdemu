#include "error.h"

#include <stdarg.h>
#include <stddef.h>

static void (*error_callback)(const char *format, va_list arg);

void SetErrorCallback(void (*error_callback_)(const char *format, va_list arg))
{
	/* TODO - Shouldn't this use the regular state instead of global state? */
	error_callback = error_callback_;
}

void PrintError(const char *format, ...)
{
	if (error_callback != NULL)
	{
		va_list args;
		va_start(args, format);
		error_callback(format, args);
		va_end(args);
	}
}
