#include "log.h"

#include <stdarg.h>
#include <stddef.h>

static void (*log_callback)(void *user_data, const char *format, va_list arg);
static void *log_callback_user_data;

void SetLogCallback(void (* const error_callback_)(void *user_data, const char *format, va_list arg), const void* const user_data)
{
	log_callback = error_callback_;
	log_callback_user_data = (void*)user_data;
}

void LogMessage(const char* const format, ...)
{
	if (log_callback != NULL)
	{
		va_list args;
		va_start(args, format);
		log_callback(log_callback_user_data, format, args);
		va_end(args);
	}
}
