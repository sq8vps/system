/**
 * @file mm.h
 * @brief General memory management routines
 * @ingroup mm_gen
 * 
 * Provides general/other memory management routines.
*/

#ifndef KERNEL_MM_H_
#define KERNEL_MM_H_

#include <stdint.h>
#include "defines.h"
#include "heap.h"
#include "ob/ob.h"

/**
 * @addtogroup mm Memory management
 */

/**
 * @addtogroup mm_gen General memory management routines
 * @ingroup mm
 * @{
*/

DRIVER_API

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
    PADDRESS physical; /**< Physical base address */
    /**
     * @brief Virtual (mapping) address of this region
     * 
     * This value is based on what was provided to \ref MmBuildMemoryDescriptorList.
     * If both producer and consumer are in the same address space, it should be valid, as long as it's not unmapped beforehand.
     * There is no guarantee that this address is correct or even accessible by the consumer.
     * The consumer may use \ref HalMapMemoryDescriptorList to make sure the regions are mapped correctly.
     */
    void *mapped;
    size_t size; /**< Region size */

    struct MmMemoryDescriptor *next; /**< Next region descriptor in a list */
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
struct MmMemoryDescriptor* MmBuildMemoryDescriptorList(void *memory, size_t size);


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
STATUS MmAllocateMemory(uintptr_t address, size_t size, MmMemoryFlags flags);

/**
 * @brief Allocate memory from the given pool and map it
 * @param address Address to map the memory to
 * @param size Memory size in bytes
 * @param flags Page flags
 * @param pool Memory pool
 * @return Error code
*/
STATUS MmAllocateMemoryFromPool(uintptr_t address, size_t size, MmMemoryFlags flags, size_t pool);

/**
 * @brief Allocate, map and zero-initialize memory
 * @param address Address to map the memory to
 * @param size Memory size in bytes
 * @param flags Page flags
 * @return Error code
*/
STATUS MmAllocateMemoryZeroed(uintptr_t address, size_t size, MmMemoryFlags flags);

/**
 * @brief Allocate memory from the given memory pool, then map and zero-initialize it
 * @param address Address to map the memory to
 * @param size Memory size in bytes
 * @param flags Page flags
 * @param pool Memory pool
 * @return Error code
*/
STATUS MmAllocateMemoryFromPoolZeroed(uintptr_t address, size_t size, MmMemoryFlags flags, size_t pool);

/**
 * @brief Unmap and free  memory
 * @param address Address to unmap and free
 * @param size Memory size in bytes
 * @return Error code
*/
STATUS MmFreeMemory(uintptr_t address, size_t size);

END_DRIVER_API

/**
 * @brief Initialize Memory Description cache allocator
 * @kinternal
*/
INTERNAL STATUS MmInitializeMemoryDescriptorAllocator(void);

/**
 * @}
*/

#endif