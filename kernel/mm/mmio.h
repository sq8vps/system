/**
 * @file mmio.h
 * @brief Memory-mapped I/O support
 * @ingroup mm_mmio
 */

#ifndef KERNEL_MMIO_H_
#define KERNEL_MMIO_H_

#include <stdint.h>
#include "defines.h"

/**
 * @addtogroup mm_mmio Memory-mapped I/O
 * @{
 */

DRIVER_API

/**
 * @brief Map Memory-Mapped I/O space
 * @param pAddress Physical address
 * @param n Space size
 * @return Pointer to mapped space or NULL on failure
*/
void *MmMapMmIo(PADDRESS pAddress, size_t n);


/**
 * @brief Unmap Memory-Mapped I/O space
 * @param *ptr Mapped memory pointer (returned from MmMapMmIo())
*/
void MmUnmapMmIo(const void *ptr);

END_DRIVER_API

/**
 * @}
 */

#endif