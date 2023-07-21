#ifndef KERNEL_MM_H_
#define KERNEL_MM_H_

/**
 * @file mm.h
 * @brief General memory management routines
 * 
 * Provides general/other memory management routines.
 * 
 * @defgroup mm Memory management module
*/

#include <stdint.h>
#include "defines.h"
#include "valloc.h"

/**
 * @defgroup genmem General memory manipulation routines
 * @ingroup mm
 * @{
*/

/**
 * @brief Allocate and map memory
 * @param address Address to map the memory to
 * @param size Memory size in bytes
 * @param flags Page flags
 * @return Error code
*/
STATUS MmAllocateMemory(uintptr_t address, uintptr_t size, MmPagingFlags_t flags);

/**
 * @brief Unmap and free  memory
 * @param address Address to unmap and free
 * @param size Memory size in bytes
 * @return Error code
*/
STATUS MmFreeMemory(uintptr_t address, uintptr_t size);

/**
 * @}
*/

#endif