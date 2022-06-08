#ifndef ERROR_H
#define ERROR_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Prevent errors when '__attribute__' is not supported. */
/* GCC 3.2 is the earliest version of GCC of which I can find proof of supporting this. */
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 2))
#define GCC_ATTRIBUTE(x) __attribute__(x)
#else
#define GCC_ATTRIBUTE(x)
#endif

void SetErrorCallback(void (*error_callback)(const char *format, va_list arg));
GCC_ATTRIBUTE((format(printf, 1, 2))) void PrintError(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* ERROR_H */
