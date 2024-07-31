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

/**
 * @defgroup genmem General memory manipulation routines
 * @ingroup mm
 * @{
*/

EXPORT
/**
 * @brief Memory flags container
 */
typedef uint16_t MmMemoryFlags;

EXPORT
#define MM_FLAG_PRESENT 1 /**< Memory is present */
#define MM_FLAG_WRITABLE 2 /**< Memory is writable */
#define MM_FLAG_USER_MODE 4 /**< Memory is available in user mode */
#define MM_FLAG_WRITE_THORUGH 8 /**< Memory is write trough */
#define MM_FLAG_CACHE_DISABLE 16 /**< Memory caching is disabled */

EXPORT
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
 * @brief Initialize Memory Description cache allocator
*/
INTERNAL STATUS MmInitializeMemoryDescriptorAllocator(void);

EXPORT
/**
 * @brief Allocate Memory Descriptor from cached pool
 * @return Memory Descriptor pointer or NULL on failure
*/
EXTERN struct MmMemoryDescriptor* MmAllocateMemoryDescriptor(void);

EXPORT
/**
 * @brief Free previously allocated Memory Descriptor
 * @param *descriptor Memory descriptor allocated with \a MmAllocateMemoryDescriptor()
 * @note This function is NULL safe
*/
EXTERN void MmFreeMemoryDescriptor(struct MmMemoryDescriptor *descriptor);

EXPORT
/**
 * @brief Build Memory Descriptor list for provided virtual memory
 * @param *memory Memory pointer
 * @param size Memory size
 * @return Memory Descriptor list pointer or NULL on failure
 * @warning This function returns NULL if size is zero
*/
EXTERN struct MmMemoryDescriptor* MmBuildMemoryDescriptorList(void *memory, uintptr_t size);

EXPORT
/**
 * @brief Free Memory Descriptor list
 * @param *list First descriptor, i.e, the list obtained from \a MmBuildMemoryDescriptorList()
 * @note This function is NULL safe
*/
EXTERN void MmFreeMemoryDescriptorList(struct MmMemoryDescriptor *list);

EXPORT
/**
 * @brief Get size of memory described by the Memory Descriptor list
 * @param *list First descriptor, i.e, the list obtained from \a MmBuildMemoryDescriptorList()
 * @return Size of memory descibed by the list
*/
EXTERN uint64_t MmGetMemoryDescriptorListSize(struct MmMemoryDescriptor *list);

EXPORT
/**
 * @brief Map memory described by the Memory Descriptor list
 * @param *list First descriptor, i.e., the list obtained from \a MmBuildMemoryDescriptorList()
 * @return Pointer to mapped dynamic memory or NULL on failure
*/
EXTERN void *HalMapMemoryDescriptorList(struct MmMemoryDescriptor *list);

EXPORT
/**
 * @brief Unmap memory previously mapped using Memory Descriptor list
 * @param *memory Virtual memory pointer
*/
EXTERN void HalUnmapMemoryDescriptorList(void *memory);

EXPORT
/**
 * @brief Copy Memory Descriptor list
 * @param *list List to be copied
 * @return Cloned Memory Descriptor List
 */
EXTERN struct MmMemoryDescriptor *MmCloneMemoryDescriptorList(struct MmMemoryDescriptor *list);

EXPORT
/**
 * @brief Allocate and map memory
 * @param address Address to map the memory to
 * @param size Memory size in bytes
 * @param flags Page flags
 * @return Error code
*/
EXTERN STATUS MmAllocateMemory(uintptr_t address, uintptr_t size, MmMemoryFlags flags);

EXPORT
/**
 * @brief Unmap and free  memory
 * @param address Address to unmap and free
 * @param size Memory size in bytes
 * @return Error code
*/
EXTERN STATUS MmFreeMemory(uintptr_t address, uintptr_t size);

/**
 * @}
*/

#endif