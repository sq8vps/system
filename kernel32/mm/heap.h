#ifndef KERNEL32_HEAP_H_
#define KERNEL32_HEAP_H_

/**
 * @file heap.h
 * @brief Heap management module
 * 
 * This module provides heap management.
 * 
 * @defgroup heap Heap management
 * @ingroup mm
*/


#include <stdint.h>
#include "../defines.h"

/**
 * @defgroup kernelHeap Kernel mode heap management routines
 * @ingroup heap
 * @{
*/

/**
 * @brief Allocate memory on kernel heap
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
*/
void *Mm_allocateKernelHeap(uintptr_t n);

/**
 * @brief Free memory allocated on kernel heap
 * @param ptr Allocated memory address
*/
void Mm_freeKernelHeap(const void *ptr);

/**
 * @}
*/

#endif