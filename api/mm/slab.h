//This header file is generated automatically
#ifndef EXPORTED___API__MM_SLAB_H_
#define EXPORTED___API__MM_SLAB_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "defines.h"
#include <stdint.h>
/**
 * @brief Create slab cache
 * @param chunkSize Slab chunk size
 * @param chunkCount Number of chunks in a slab
 * @return Slab handle or NULL on failure
 * @attention This function fails when either parameter is zero
*/
extern void *MmSlabCreate(uintptr_t chunkSize, uintptr_t chunkCount);

/**
 * @brief Allocate memory chunk
 * @param *slabHandle Slab handle obtained from \a MmSlabCreate()
 * @return Allocated chuck or NULL on failure
*/
extern void *MmSlabAllocate(void *slabHandle);

/**
 * @brief Free memory chunk
 * @param *slabHandle Slab handle obtained from \a MmSlabCreate()
 * @param *memory Chunk pointer
 * @note This function does nothing if \a memory is NULL
*/
extern void MmSlabFree(void *slabHandle, void *memory);


#ifdef __cplusplus
}
#endif

#endif