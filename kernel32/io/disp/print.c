#include "print.h"
#include "common/vprintf.h"

int IoVprint(const char *format, va_list args)
{
    struct VPrintfConfig c;
    c.toFile = true;
    c.useMax = false;
    int ret = CmVprintf(c, format, args);
    return ret;
}

__attribute__ ((format (printf, 1, 2)))
int IoPrint(const char *format, ...)
{
	va_list args;
	va_start(args, format);
    int ret = IoVprint(format, args);
    va_end(args);
    return ret;
}

__attribute__ ((format (printf, 2, 3)))
int IoPrintN(size_t n, const char *format, ...)
{
    struct VPrintfConfig c;
    c.toFile = true;
    c.useMax = true;
    c.max = n;
	va_list args;
	va_start(args, format);
    int ret = CmVprintf(c, format, args);
    va_end(args);
    return ret;
}

__attribute__ ((format (printf, 2, 3)))
int IoSprint(char *s, const char *format, ...)
{
    struct VPrintfConfig c;
    c.toFile = false;
    c.buffer = s;
    c.useMax = false;
	va_list args;
	va_start(args, format);
    int ret = CmVprintf(c, format, args);
    va_end(args);
    return ret;
}

__attribute__ ((format (printf, 3, 4)))
int IoSprintN(char *s, size_t n, const char *format, ...)
{
    struct VPrintfConfig c;
    c.toFile = false;
    c.buffer = s;
    c.useMax = true;
    c.max = n - 1;
	va_list args;
	va_start(args, format);
    int ret = CmVprintf(c, format, args);
    va_end(args);
    return ret;
}
