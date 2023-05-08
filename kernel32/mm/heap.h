#ifndef KERNEL32_HEAP_H_
#define KERNEL32_HEAP_H_

#include <stdint.h>
#include "../defines.h"

/**
 * @brief Allocate memory on kernel heap
 * @param n Count of bytes to allocate
 * @return Pointer to allocated memory or NULL on failure
*/
void *Mm_allocateKernelHeap(uint32_t n);

/**
 * @brief Free memory allocated on kernel heap
 * @param ptr Allocated memory address
*/
void Mm_freeKernelHeap(void *ptr);

#endif