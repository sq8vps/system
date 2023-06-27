#ifndef KERNEL32_HEAP_H_
#define KERNEL32_HEAP_H_

/**
 * @file heap.h
 * @brief Kernel heap management module
 * 
 * This module provides kernel heap management.
 * 
 * @ingroup mm
*/


#include <stdint.h>
#include "../defines.h"

/**
 * @defgroup kernelHeap Kernel mode heap management routines
 * @ingroup mm
 * @{
*/

/**
 * @brief Allocate memory on kernel heap
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
*/
EXPORT void *MmAllocateKernelHeap(uintptr_t n);

/**
 * @brief Free memory allocated on kernel heap
 * @param ptr Allocated memory address
*/
EXPORT void MmFreeKernelHeap(const void *ptr);

/**
 * @}
*/

#endif