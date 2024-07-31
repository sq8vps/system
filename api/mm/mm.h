//This header file is generated automatically
#ifndef EXPORTED___API__MM_MM_H_
#define EXPORTED___API__MM_MM_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "heap.h"
/**
 * @brief Memory flags container
 */
typedef uint16_t MmMemoryFlags;

#define MM_FLAG_PRESENT 1 /**< Memory is present */
#define MM_FLAG_WRITABLE 2 /**< Memory is writable */
#define MM_FLAG_USER_MODE 4 /**< Memory is available in user mode */
#define MM_FLAG_WRITE_THORUGH 8 /**< Memory is write trough */
#define MM_FLAG_CACHE_DISABLE 16 /**< Memory caching is disabled */

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
extern struct MmMemoryDescriptor* MmAllocateMemoryDescriptor(void);

/**
 * @brief Free previously allocated Memory Descriptor
 * @param *descriptor Memory descriptor allocated with \a MmAllocateMemoryDescriptor()
 * @note This function is NULL safe
*/
extern void MmFreeMemoryDescriptor(struct MmMemoryDescriptor *descriptor);

/**
 * @brief Build Memory Descriptor list for provided virtual memory
 * @param *memory Memory pointer
 * @param size Memory size
 * @return Memory Descriptor list pointer or NULL on failure
 * @warning This function returns NULL if size is zero
*/
extern struct MmMemoryDescriptor* MmBuildMemoryDescriptorList(void *memory, uintptr_t size);

/**
 * @brief Free Memory Descriptor list
 * @param *list First descriptor, i.e, the list obtained from \a MmBuildMemoryDescriptorList()
 * @note This function is NULL safe
*/
extern void MmFreeMemoryDescriptorList(struct MmMemoryDescriptor *list);

/**
 * @brief Get size of memory described by the Memory Descriptor list
 * @param *list First descriptor, i.e, the list obtained from \a MmBuildMemoryDescriptorList()
 * @return Size of memory descibed by the list
*/
extern uint64_t MmGetMemoryDescriptorListSize(struct MmMemoryDescriptor *list);

/**
 * @brief Map memory described by the Memory Descriptor list
 * @param *list First descriptor, i.e., the list obtained from \a MmBuildMemoryDescriptorList()
 * @return Pointer to mapped dynamic memory or NULL on failure
*/
extern void *HalMapMemoryDescriptorList(struct MmMemoryDescriptor *list);

/**
 * @brief Unmap memory previously mapped using Memory Descriptor list
 * @param *memory Virtual memory pointer
*/
extern void HalUnmapMemoryDescriptorList(void *memory);

/**
 * @brief Copy Memory Descriptor list
 * @param *list List to be copied
 * @return Cloned Memory Descriptor List
 */
extern struct MmMemoryDescriptor *MmCloneMemoryDescriptorList(struct MmMemoryDescriptor *list);

/**
 * @brief Allocate and map memory
 * @param address Address to map the memory to
 * @param size Memory size in bytes
 * @param flags Page flags
 * @return Error code
*/
extern STATUS MmAllocateMemory(uintptr_t address, uintptr_t size, MmMemoryFlags flags);

/**
 * @brief Unmap and free  memory
 * @param address Address to unmap and free
 * @param size Memory size in bytes
 * @return Error code
*/
extern STATUS MmFreeMemory(uintptr_t address, uintptr_t size);


#ifdef __cplusplus
}
#endif

#endif