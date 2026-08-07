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

DRIVER_API

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
 * @param *slabHandle Slab handle obtained from \ref MmSlabCreate()
 * @return Allocated chuck or NULL on failure
*/
void *MmSlabAllocate(void *slabHandle);


/**
 * @brief Free memory chunk
 * @param *slabHandle Slab handle obtained from \ref MmSlabCreate()
 * @param *memory Chunk pointer
 * @note This function does nothing if \a memory is NULL
*/
void MmSlabFree(void *slabHandle, void *memory);

/**
 * @brief Allocate memory chunk and return its physical address
 * @param *slabHandle Slab handle obtained from \ref MmSlabCreateP()
 * @param *physical Memory to store the physical address to
 * @return Allocated chuck or nullptr on failure
*/
void *MmSlabAllocateP(void *slabHandle, PADDRESS *physical);

/**
 * @brief Free memory chunk with a known physical address
 * @param *slabHandle Slab handle obtained from \ref MmSlabCreateP()
 * @param *memory Chunk pointer
 * @param physical Chunk physical address
 * @note This function does nothing if \a memory is nullptr
*/
void MmSlabFreeP(void *slabHandle, void *memory, PADDRESS physical);

/**
 * @brief Create slab cache in a given physical memory pool
 * @param chunkSize Slab chunk size
 * @param pool Physical memory pool
 * @param zero Set to zero
 * @return Slab handle or nullptr on failure
*/
void *MmSlabCreateP(size_t chunkSize, uint32_t pool, size_t zero);

/**
 * @brief Destroy slab cache
 * @param *slabHandle Slab handle obtained from MmSlabCreate()
 */
void MmSlabDestroy(void *slabHandle);

END_DRIVER_API

/**
 * @}
 */

#endif