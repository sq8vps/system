#include "print.h"
#include "common/stdio.h"

__attribute__ ((format (printf, 1, 2)))
int IoPrint(const char *format, ...)
{
	va_list args;
	va_start(args, format);
    int ret = CmVprintf(format, args);
    va_end(args);
    return ret;
}

__attribute__ ((format (printf, 1, 2)))
int IoPrintDebug(const char *format, ...)
{
#ifdef DEBUG
	va_list args;
	va_start(args, format);
    int ret = CmVprintf(format, args);
    va_end(args);
    return ret;
#else
    return 0;
#endif
}

int IoVprint(const char *format, va_list args)
{
    return CmVprintf(format, args);
}

int IoVprintDebug(const char *format, va_list args)
{
#ifdef DEBUG
    return CmVprintf(format, args);
#else
    return 0;
#endif
}