#ifndef HAL_ARCH_H_
#define HAL_ARCH_H_

#include "defines.h"

EXPORT_API

//include macros and typedefs defined by architectures
#if defined(__i686__) || defined(__amd64__)
#include "i686/i686.h"
#endif

END_EXPORT_API

#endif