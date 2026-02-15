#ifndef KERNEL_HEAP_H_
#define KERNEL_HEAP_H_

/**
 * @file heap.h
 * @brief Kernel heap management module
 * 
 * This module provides kernel heap management.
 * 
 * @ingroup mm_heap
*/


#include <stdint.h>
#include "defines.h"

/**
 * @addtogroup mm_heap Kernel mode heap management routines
 * @ingroup mm
 * @{
*/

DRIVER_API

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

/**
 * @brief Allocate aligned memory on kernel heap
 * @param n Count of bytes to allocate
 * @param align Alignment in bytes, must be a power of 2
 * @return Pointer to allocated memory or NULL on failure
*/
MALLOC_LIKE(MmFreeKernelHeap)
void *MmAllocateKernelHeapAligned(size_t n, size_t align);


/**
 * @brief Allocate memory on kernel heap
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 * @note The address returned is aligned to a non-zero multiple of 16 bytes
*/
MALLOC_LIKE(MmFreeKernelHeap)
void *MmAllocateKernelHeap(size_t n);


/**
 * @brief Allocate memory on kernel heap and clear it
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 * @note The address returned is aligned to a non-zero multiple of 16 bytes
*/
MALLOC_LIKE(MmFreeKernelHeap)
void *MmAllocateKernelHeapZeroed(size_t n);

/**
 * @brief Reallocate memory on kernel heap
 * @param n Count of bytes to reallocate
 * @return Pointer to allocated memory or NULL on failure
 * @note The address returned is aligned to a non-zero multiple of 16 bytes
*/
MALLOC_LIKE(MmFreeKernelHeap)
void *MmReallocateKernelHeap(void *ptr, size_t n);

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
 * @brief Reallocate memory on kernel heap
 * @param n Count of bytes to reallocate
 * @return Pointer to allocated memory or NULL on failure
 * @note The address returned is aligned to a non-zero multiple of 16 bytes
*/
#define realloc(ptr, n) MmReallocateKernelHeap(ptr, n);


END_DRIVER_API

/**
 * @}
*/

#endif