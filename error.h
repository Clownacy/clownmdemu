#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Prevent errors when __attribute__ is not supported. */
#ifndef __GNUC__
#define  __attribute__(x)
#endif

void SetErrorCallback(void (*error_callback)(const char *format, va_list arg));
__attribute__((format(printf, 1, 2))) void PrintError(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* ERROR_H */
