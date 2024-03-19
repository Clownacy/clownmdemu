#include "error.h"

#include <stdarg.h>
#include <stddef.h>

static void (*error_callback)(void *user_data, const char *format, va_list arg);
static void *error_callback_user_data;

void SetErrorCallback(void (* const error_callback_)(void *user_data, const char *format, va_list arg), const void* const user_data)
{
	error_callback = error_callback_;
	error_callback_user_data = (void*)user_data;
}

void PrintError(const char* const format, ...)
{
	if (error_callback != NULL)
	{
		va_list args;
		va_start(args, format);
		error_callback(error_callback_user_data, format, args);
		va_end(args);
	}
}
