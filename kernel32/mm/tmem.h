#ifndef KERNEL_TMEM_H_
#define KERNEL_TMEM_H_

#include "defines.h"
#include <stdint.h>
#include "ob/ob.h"

EXPORT_API

struct IoFileHandle;

/**
 * @brief Flags for \a MmTaskMemory descriptor
 */
enum MmTaskMemoryFlags
{
    MM_TASK_MEMORY_READABLE = 0x1, /**< Region is readable */
    MM_TASK_MEMORY_WRITABLE = 0x2, /**< Region is writable */
    MM_TASK_MEMORY_EXECUTABLE = 0x4, /**< Region is executable */
    MM_TASK_MEMORY_SHARED = 0x8, /**< Region is shared, write directly to underyling buffer */
    MM_TASK_MEMORY_WRITE_THROUGH = MM_TASK_MEMORY_SHARED, /**< Write directly to mapped file rather than to a private buffer */
    MM_TASK_MEMORY_REVERSED = 0x10, /**< Region grows down - the top pointer is returned and the top pointer is expected when mapping and unmapping.
                                        This flag is illegal for file mappings. */
    MM_TASK_MEMORY_GROWABLE = 0x20, /**< Region can grow beyond its original size - downwards if \a MM_TASK_MEMORY_REVERSED is set */
    MM_TASK_MEMORY_LOCKED = 0x40, /**< Region is a fundamental process memory which can't be removed */
    MM_TASK_MEMORY_FIXED = 0x80, /**< Map memory exactly to given address, fail if not possible. Fixed allocations do not have guard pages unless they are growable. */
    
    MM_TASK_MEMORY_STACK = MM_TASK_MEMORY_READABLE | MM_TASK_MEMORY_WRITABLE | MM_TASK_MEMORY_REVERSED, /**< A set of flags for allocating stacks */
};

/**
 * @brief General task memory region descriptor
 */
struct MmTaskMemory
{
    OBJECT;
    void *base; /**< Region start address */
    void *end; /**< Region end address */
    size_t limit; /**< Size limit if \a MM_TASK_MEMORY_GROWABLE is specified - 0 implies no limit */
    enum MmTaskMemoryFlags flags; /**< Region flags */
    struct IoFileHandle *file; /**< Associated file */
    uint64_t offset; /**< Offset within the associated file */

    void *treeData[8]; /**< Masked tree node data to avoid namespace pollution */
    struct MmTaskMemory *next, *previous; /**< Linked list of base-ordered memory entries associated with given process */
};

/**
 * @brief Check whether a memory starting at \a base of size \a size is user-space memory
 * @param base Memory base
 * @param size Memory size
 * @return True if user-space memory, false otherwise
 * @note This macro accounts for address wrapping
 */
#define IS_USER_MEMORY(base, size) ((((uintptr_t)(base) + (uintptr_t)(size)) < HAL_USER_SPACE_TOP) && (((uintptr_t)(base) + (uintptr_t)(size)) >= (uintptr_t)(base)))

/**
 * @brief Map task memory
 * @param *address Address to map to. If NULL, the kernel selects a suitable page-aligned address. 
 * If not NULL, the kernel *might* take a hint about where to place the mapping. If \a MM_TASK_MEMORY_FIXED flag is specified, 
 * then mapping is placed exactly at the specified address. If it's not possible (bad alignment, region already mapped, etc.) the function fails.
 * If \a MM_TASK_MEMORY_REVERSED flag is specified, the address must point to the *top* (highest) address of the mapping.
 * @param size Size of the mapping. The actual mapping size is rounded up to the page size. If \a fd (file descriptor) is provided, then \a size
 * specified the number of bytes to read starting at \a offset from the file.
 * @param flags Mapping flags. Refer to #MmTaskMemoryFlags
 * @param fd Descriptor of the file to be mapped. If negative (<0), a zero-initialized memory mapping, not backed by any file, is created. 
 * File open mode must not conflict with provided flags.
 * @param offset Offset within the file, must be page aligned. If file is not provided, then it is ignored.
 * @param limit Region size limit if \a MM_TASK_MEMORY_GROWABLE flag is specified. The limit does not include the guard page size.
 * @param **mapped Mapped region pointer. If \a MM_TASK_MEMORY_REVERSED flag is specified, the pointer referes to the *top* (highest) address of the mapping.
 * If the returned status is not \a OK, the value is invalid. NULL can be provided if this pointer is not needed.
 * @return Status code
 */
STATUS MmMapTaskMemory(void *address, size_t size, enum MmTaskMemoryFlags flags, int fd, uint64_t offset, size_t limit, void **mapped);

/**
 * @brief Get task memory descriptor for given pointer
 * @param *ptr Pointer of which memory descriptor is to be found
 * @return Memory descriptor associated with \a *ptr or NULL if not found
 */
struct MmTaskMemory *MmGetTaskMemoryDescriptor(const void *const ptr);

END_EXPORT_API

#endif