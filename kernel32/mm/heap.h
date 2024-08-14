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

EXPORT_API

/**
 * @brief Allocate aligned memory on kernel heap
 * @param n Count of bytes to allocate
 * @param align Alignment in bytes, must be a power of 2
 * @return Pointer to allocated memory or NULL on failure
*/
void *MmAllocateKernelHeapAligned(uintptr_t n, uintptr_t align);


/**
 * @brief Allocate memory on kernel heap
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 * @note The address returned is aligned to a non-zero multiple of 16 bytes
*/
void *MmAllocateKernelHeap(uintptr_t n);


/**
 * @brief Allocate memory on kernel heap and clear it
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 * @note The address returned is aligned to a non-zero multiple of 16 bytes
*/
void *MmAllocateKernelHeapZeroed(uintptr_t n);


/**
 * @brief Allocate memory on kernel heap
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
*/
#define malloc(n) MmAllocateKernelHeap(n)


/**
 * @brief Allocate memory on kernel heap and clear it
 * @param n Number of elements to allocate
 * @param size Element size
 * @return Pointer to allocated memory or NULL on failure
 * @note The address returned is aligned to a non-zero multiple of 16 bytes
*/
#define calloc(n, size) MmAllocateKernelHeapZeroed((n) * (size))


/**
 * @brief Free memory allocated on kernel heap
 * @param ptr Allocated memory address
*/
void MmFreeKernelHeap(const void *ptr);


/**
 * @brief Free memory allocated on kernel heap
 * @param ptr Allocated memory address
*/
#define free(ptr) MmFreeKernelHeap(ptr)

END_EXPORT_API

/**
 * @}
*/

#endif