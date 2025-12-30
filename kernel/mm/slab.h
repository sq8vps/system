/**
 * @file slab.h
 * @brief Slab memory allocator
 * @ingroup mm_slab
 */

#ifndef MM_SLAB_H_
#define MM_SLAB_H_

#include "defines.h"
#include <stdint.h>

/**
 * @addtogroup mm_slab Slab memory allocator
 * @ingroup mm
 * 
 * This module aims to provide very fast kernel memory allocation.
 * @{ 
 */

EXPORT_API

/**
 * @brief Create slab cache
 * @param chunkSize Slab chunk size
 * @param chunkCount Number of chunks in a slab
 * @return Slab handle or NULL on failure
 * @attention This function fails when either parameter is zero
*/
void *MmSlabCreate(size_t chunkSize, size_t chunkCount);


/**
 * @brief Allocate memory chunk
 * @param *slabHandle Slab handle obtained from \a MmSlabCreate()
 * @return Allocated chuck or NULL on failure
*/
void *MmSlabAllocate(void *slabHandle);


/**
 * @brief Free memory chunk
 * @param *slabHandle Slab handle obtained from \a MmSlabCreate()
 * @param *memory Chunk pointer
 * @note This function does nothing if \a memory is NULL
*/
void MmSlabFree(void *slabHandle, void *memory);

/**
 * @brief Destroy slab cache
 * @param *slabHandle Slab handle obtained from MmSlabCreate()
 */
void MmSlabDestroy(void *slabHandle);

END_EXPORT_API

/**
 * @}
 */

#endif