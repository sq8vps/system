//This header file is generated automatically
#ifndef EXPORTED___SYS__MMAP_H_
#define EXPORTED___SYS__MMAP_H_

#ifdef __cplusplus
extern "C" 
{
#endif


#include <stddef.h>
#include <stdint.h>

#define SYSCALL_MMAP 6 /**< Map files or devices into memory */
#define SYSCALL_EXMMAP 7 /**< Extended mmap with 64-bit offset and size limit */
#define SYSCALL_MUNMAP 8 /**< Unmap memory */
#define SYSCALL_SET_TLS 9 /**< Set Thread-local Storage pointer */

/**
 * @brief @ref mmap() and @ref exmmap() flags
 */
enum
{
    MMAP_READABLE = 0x1, /**< Region is readable */
    MMAP_WRITABLE = 0x2, /**< Region is writable */
    MMAP_EXECUTABLE = 0x4, /**< Region is executable */
    MMAP_FILE = 0x8, /**< Region is backed up by a file */
    MMAP_REVERSED = 0x10, /**< Region grows down - the top pointer is returned and the top pointer is expected when mapping and unmapping.
                                        This flag is illegal for file mappings. */
    MMAP_GROWABLE = 0x20, /**< Region can grow beyond its original size - downwards if \a MMAP_REVERSED is set */
    MMAP_FIXED = 0x40, /**< Map memory exactly to given address, fail if not possible. Fixed allocations do not have guard pages unless they are growable. */
    
    MMAP_STACK = MMAP_READABLE | MMAP_WRITABLE | MMAP_REVERSED, /**< A set of flags for allocating stacks */
};

/**
 * @brief Map files or devices into memory
 * @param addr Desired address (or NULL for automatic selection)
 * @param length Length of mapping
 * @param flags Mapping flags
 * @param fd File descriptor to map. Don't care if MMAP_FILE flag is not set.
 * @param offset Offset in file/device
 * @return Mapped address on success, MAP_FAILED on failure
 * @note This system call does not allow setting alignment. Use @ref exmmap() for that purpose.
 * @note This system call does not allow setting limit of the growable mapping. Use @ref exmmap() for that purpose.
 * @note This system call uses native, register-sized offset. To explicitly use 64-bit offsets, use @ref exmmap() instead.
 */
void *mmap(void *addr, size_t length, int flags, int fd, size_t offset);

/**
 * @brief Extended mmap parameters
 */
struct exmmap_params
{
    size_t alignment; /**< Required base alignment */
    size_t limit; /**< Size limit of growable mapping */
    uint64_t offset; /**< 64-bit offset */
};

/**
 * @brief Map files or devices into memory with extended features
 * @param addr Desired address (or NULL for automatic selection)
 * @param length Length of mapping
 * @param flags Mapping flags
 * @param fd File descriptor to map. Don't care if MMAP_FILE flag is not set.
 * @param *params Extended mmap parameters (see @ref exmmap_params). Setting to NULL implies default alignment, zero offset and no size limit.
 * @return Mapped address on success, MAP_FAILED on failure
 */
void *exmmap(void *addr, size_t length, int flags, int fd, const struct exmmap_params *const params);

/**
 * @brief Unmap previously mapped region(s)
 * @param *addr Pointer withtin the region to be removed
 * @param length Number of bytes to look for the regions to be unmapped starting from *addr
 * @return 0 on success, -1 on failure
 */
int munmap(void *addr, size_t length);

/**
 * @brief Set Thread-local Storage pointer
 * @param *tls TLS pointer
 */
void set_thread_local_storage(void *tls);


#ifdef __cplusplus
}
#endif

#endif