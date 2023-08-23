#ifndef KERNEL_HEAP_H_
#define KERNEL_HEAP_H_

/**
 * @file heap.h
 * @brief Kernel heap management module
 * 
 * This module provides kernel heap management.
 * 
 * @ingroup mm
*/


#include <stdint.h>
#include "defines.h"

/**
 * @defgroup kernelHeap Kernel mode heap management routines
 * @ingroup mm
 * @{
*/

EXPORT
/**
 * @brief Allocate memory on kernel heap
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
*/
EXTERN void *MmAllocateKernelHeap(uintptr_t n);

EXPORT
/**
 * @brief Free memory allocated on kernel heap
 * @param ptr Allocated memory address
*/
EXTERN void MmFreeKernelHeap(const void *ptr);

/**
 * @}
*/

#endif