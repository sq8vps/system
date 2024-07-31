#ifndef HAL_ARCHDEFS_H_
#define HAL_ARCHDEFS_H_

//include macros and typedefs defined by architectures
#if defined(__i686__) || defined(__amd64__)
#include "i686/i686.h"
#endif

#endif