#ifndef KERNEL_MM_H_
#define KERNEL_MM_H_

/**
 * @file mm.h
 * @brief General memory management routines
 * 
 * Provides general/other memory management routines.
 * 
 * @defgroup mm Memory management module
*/

#include <stdint.h>
#include "defines.h"
#include "heap.h"
#include "ob/ob.h"

/**
 * @defgroup genmem General memory manipulation routines
 * @ingroup mm
 * @{
*/

EXPORT_API

struct IoFileHandle;

/**
 * @brief Memory flags container
 */
typedef uint16_t MmMemoryFlags;


#define MM_FLAG_PRESENT 1 /**< Memory is present */
#define MM_FLAG_WRITABLE 2 /**< Memory is writable */
#define MM_FLAG_USER_MODE 4 /**< Memory is available in user mode */
#define MM_FLAG_WRITE_THROUGH 8 /**< Memory is write trough */
#define MM_FLAG_CACHE_DISABLE 16 /**< Memory caching is disabled */
#define MM_FLAG_READ_ONLY 32 /**< Memory is read-only (which is default). This flag supersedes \a MM_FLAG_WRITABLE if specified */
#define MM_FLAG_EXECUTABLE 64 /**< Memory is explicitly executable. This might or might not be implemented */
#define MM_FLAG_NON_EXECUTABLE 128 /**< Memory is explictly non-executable. This might or might not be implemented */

/**
 * @brief A descriptor for physical memory region
*/
struct MmMemoryDescriptor
{
    uintptr_t physical;
    void *mapped;
    uintptr_t size;

    struct MmMemoryDescriptor *next;
};

/**
 * @brief Allocate Memory Descriptor from cached pool
 * @return Memory Descriptor pointer or NULL on failure
*/
struct MmMemoryDescriptor* MmAllocateMemoryDescriptor(void);

/**
 * @brief Free previously allocated Memory Descriptor
 * @param *descriptor Memory descriptor allocated with \a MmAllocateMemoryDescriptor()
 * @note This function is NULL safe
*/
void MmFreeMemoryDescriptor(struct MmMemoryDescriptor *descriptor);


/**
 * @brief Build Memory Descriptor list for provided virtual memory
 * @param *memory Memory pointer
 * @param size Memory size
 * @return Memory Descriptor list pointer or NULL on failure
 * @warning This function returns NULL if size is zero
*/
struct MmMemoryDescriptor* MmBuildMemoryDescriptorList(void *memory, uintptr_t size);


/**
 * @brief Free Memory Descriptor list
 * @param *list First descriptor, i.e, the list obtained from \a MmBuildMemoryDescriptorList()
 * @note This function is NULL safe
*/
void MmFreeMemoryDescriptorList(struct MmMemoryDescriptor *list);


/**
 * @brief Get size of memory described by the Memory Descriptor list
 * @param *list First descriptor, i.e, the list obtained from \a MmBuildMemoryDescriptorList()
 * @return Size of memory descibed by the list
*/
uint64_t MmGetMemoryDescriptorListSize(struct MmMemoryDescriptor *list);


/**
 * @brief Map memory described by the Memory Descriptor list
 * @param *list First descriptor, i.e., the list obtained from \a MmBuildMemoryDescriptorList()
 * @return Pointer to mapped dynamic memory or NULL on failure
*/
void *HalMapMemoryDescriptorList(struct MmMemoryDescriptor *list);


/**
 * @brief Unmap memory previously mapped using Memory Descriptor list
 * @param *memory Virtual memory pointer
*/
void HalUnmapMemoryDescriptorList(void *memory);


/**
 * @brief Copy Memory Descriptor list
 * @param *list List to be copied
 * @return Cloned Memory Descriptor List
 */
struct MmMemoryDescriptor *MmCloneMemoryDescriptorList(struct MmMemoryDescriptor *list);


/**
 * @brief Allocate and map memory
 * @param address Address to map the memory to
 * @param size Memory size in bytes
 * @param flags Page flags
 * @return Error code
*/
STATUS MmAllocateMemory(uintptr_t address, uintptr_t size, MmMemoryFlags flags);

/**
 * @brief Allocate, map and zero-initialize memory
 * @param address Address to map the memory to
 * @param size Memory size in bytes
 * @param flags Page flags
 * @return Error code
*/
STATUS MmAllocateMemoryZeroed(uintptr_t address, uintptr_t size, MmMemoryFlags flags);

/**
 * @brief Unmap and free  memory
 * @param address Address to unmap and free
 * @param size Memory size in bytes
 * @return Error code
*/
STATUS MmFreeMemory(uintptr_t address, uintptr_t size);

END_EXPORT_API

/**
 * @brief Initialize Memory Description cache allocator
*/
INTERNAL STATUS MmInitializeMemoryDescriptorAllocator(void);

/**
 * @}
*/

#endif