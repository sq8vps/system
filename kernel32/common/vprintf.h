#ifndef KERNEL_VPRINTF_H_
#define KERNEL_VPRITNF_H_

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

struct IoFileHandle;

struct VPrintfConfig
{
	bool toFile;
	struct IoFileHandle *file;
	char *buffer;
	bool useMax;
	size_t max;
};

int CmVprintf(struct VPrintfConfig config, const char *format, va_list args) ;

#endif