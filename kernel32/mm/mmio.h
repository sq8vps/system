#ifndef KERNEL_MMIO_H_
#define KERNEL_MMIO_H_

#include <stdint.h>
#include "defines.h"

/**
 * @brief Map Memory-Mapped I/O space
 * @param pAddress Physical address
 * @param n Space size
 * @return Pointer to mapped space or NULL on failure
*/
void *MmMapMmIo(uintptr_t pAddress, uintptr_t n);

/**
 * @brief Unmap Memory-Mapped I/O space
 * @param *ptr Mapped memory pointer (returned from MmMapMmIo())
 * @param n Space size
*/
void MmUnmapMmIo(void *ptr, uintptr_t n);

#endif