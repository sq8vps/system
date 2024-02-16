#ifndef KERNEL_MMIO_H_
#define KERNEL_MMIO_H_

#include <stdint.h>
#include "defines.h"

EXPORT
/**
 * @brief Map Memory-Mapped I/O space
 * @param pAddress Physical address
 * @param n Space size
 * @return Pointer to mapped space or NULL on failure
*/
EXTERN void *MmMapMmIo(uintptr_t pAddress, uintptr_t n);

EXPORT
/**
 * @brief Unmap Memory-Mapped I/O space
 * @param *ptr Mapped memory pointer (returned from MmMapMmIo())
*/
EXTERN void MmUnmapMmIo(void *ptr);

#endif