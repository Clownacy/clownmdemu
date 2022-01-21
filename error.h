#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

void SetErrorCallback(void (*error_callback)(const char *format, va_list arg));
void PrintError(const char *format, ...);

#endif /* ERROR_H */
