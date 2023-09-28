//This header file is generated automatically
#ifndef EXPORTED___API__EX_KSYM_H_
#define EXPORTED___API__EX_KSYM_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
/**
 * @brief Resolve kernel symbol
 * @param name Kernel symbol name
 * @return Kernel symbol value
 * @attention Kernel symbol table must be loaded first using ExLoadKernelSymbols()
*/
extern uintptr_t ExGetKernelSymbol(const char *name);


#ifdef __cplusplus
}
#endif

#endif