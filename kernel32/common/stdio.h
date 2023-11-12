#ifndef KERNEL_STDIO_
#define KERNEL_STDIO_

#include "defines.h"
#include <stdarg.h>

EXPORT
EXTERN int CmVprintf(const char *format, va_list args);

#endif