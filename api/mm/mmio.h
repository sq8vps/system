//This header file is generated automatically
#ifndef EXPORTED___API__MM_MMIO_H_
#define EXPORTED___API__MM_MMIO_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
/**
 * @brief Map Memory-Mapped I/O space
 * @param pAddress Physical address
 * @param n Space size
 * @return Pointer to mapped space or NULL on failure
*/
extern void *MmMapMmIo(uintptr_t pAddress, uintptr_t n);

/**
 * @brief Unmap Memory-Mapped I/O space
 * @param *ptr Mapped memory pointer (returned from MmMapMmIo())
 * @param n Space size
*/
extern void MmUnmapMmIo(void *ptr, uintptr_t n);


#ifdef __cplusplus
}
#endif

#endif