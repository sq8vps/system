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
 * @brief Allocate memory on kernel heap
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
*/
extern void *MmAllocateKernelHeap(uintptr_t n);

/**
 * @brief Allocate memory on kernel heap
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
*/
#define malloc(n) MmAllocateKernelHeap(n)

/**
 * @brief Free memory allocated on kernel heap
 * @param ptr Allocated memory address
*/
extern void MmFreeKernelHeap(const void *ptr);

/**
 * @brief Free memory allocated on kernel heap
 * @param ptr Allocated memory address
*/
#define free(ptr) MmFreeKernelHeap(ptr)


#ifdef __cplusplus
}
#endif

#endif