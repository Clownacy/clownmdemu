#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void SetErrorCallback(void (*error_callback)(const char *format, va_list arg));
void PrintError(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* ERROR_H */
