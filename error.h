#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

#include "clowncommon/clowncommon.h"

#ifdef __cplusplus
extern "C" {
#endif

void SetErrorCallback(void (*error_callback)(const char *format, va_list arg));
CC_ATTRIBUTE_PRINTF(1, 2) void PrintError(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* ERROR_H */
