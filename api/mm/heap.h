//This header file is generated automatically
#ifndef EXPORTED___API__MM_HEAP_H_
#define EXPORTED___API__MM_HEAP_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"

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
__attribute__((malloc, malloc(MmFreeKernelHeap)))
void *MmAllocateKernelHeapAligned(uintptr_t n, uintptr_t align);


/**
 * @brief Allocate memory on kernel heap
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 * @note The address returned is aligned to a non-zero multiple of 16 bytes
*/
__attribute__((malloc, malloc(MmFreeKernelHeap)))
void *MmAllocateKernelHeap(uintptr_t n);


/**
 * @brief Allocate memory on kernel heap and clear it
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
 * @note The address returned is aligned to a non-zero multiple of 16 bytes
*/
__attribute__((malloc, malloc(MmFreeKernelHeap)))
void *MmAllocateKernelHeapZeroed(uintptr_t n);

/**
 * @brief Reallocate memory on kernel heap
 * @param n Count of bytes to reallocate
 * @return Pointer to allocated memory or NULL on failure
 * @note The address returned is aligned to a non-zero multiple of 16 bytes
*/
__attribute__((malloc, malloc(MmFreeKernelHeap)))
void *MmReallocateKernelHeap(void *ptr, uintptr_t n);

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



#ifdef __cplusplus
}
#endif

#endif