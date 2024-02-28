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
 * @note The address returned is aligned to a non-zero multiple of 16 bytes
*/
EXTERN void *MmAllocateKernelHeap(uintptr_t n);

EXPORT
/**
 * @brief Allocate memory on kernel heap and clear it
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 * @note The address returned is aligned to a non-zero multiple of 16 bytes
*/
EXTERN void *MmAllocateKernelHeapZeroed(uintptr_t n);

EXPORT
/**
 * @brief Allocate memory on kernel heap
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
*/
#define malloc(n) MmAllocateKernelHeap(n)

EXPORT
/**
 * @brief Allocate memory on kernel heap and clear it
 * @param n Number of elements to allocate
 * @param size Element size
 * @return Pointer to allocated memory or NULL on failure
 * @note The address returned is aligned to a non-zero multiple of 16 bytes
*/
#define calloc(n, size) MmAllocateKernelHeapZeroed((n) * (size))

EXPORT
/**
 * @brief Free memory allocated on kernel heap
 * @param ptr Allocated memory address
*/
EXTERN void MmFreeKernelHeap(const void *ptr);

EXPORT
/**
 * @brief Free memory allocated on kernel heap
 * @param ptr Allocated memory address
*/
#define free(ptr) MmFreeKernelHeap(ptr)

/**
 * @}
*/

#endif