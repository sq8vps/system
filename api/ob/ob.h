//This header file is generated automatically
#ifndef EXPORTED___API__OB_OB_H_
#define EXPORTED___API__OB_OB_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "ke/core/mutex.h"
#include "defines.h"

struct KeProcessControlBlock;

/**
 * @brief Kernel object types
 */
enum ObObjectType
{
    OB_UNKNOWN = 0x0, /**< Unknown object type */
    OB_SPINLOCK = 0x1, /**< Spinlock */
    OB_MUTEX = 0x2, /**< Mutex */
    OB_SEMAPHORE = 0x3, /**< Semaphore */
    OB_RW_LOCK = 0x4, /**< Read/write lock */
    OB_PCB = 0x5, /**< Process control block */
    OB_TCB = 0x6, /**< Task/thread control block */
    OB_FILE = 0x7, /**< File descriptor */
    OB_VFS_NODE = 0x08, /**< VFS node */
    OB_DRIVER = 0x09, /**< Driver object */
    OB_DEVICE = 0x0A, /**< Device object */
    OB_DEVICE_NODE = 0x0B, /**< Device node */
    OB_RP, /**< I/O request packet */
    OB_VOLUME, /**< Volume node */
    OB_SYSLOG, /**< System logger handle */
    OB_EVENT, /**< Event handler */

    OB_TYPE_COUNT, /**< Kernel object type count, do not use */
};

/**
 * @brief Header structure for all kernel objects
*/
struct ObObjectHeader
{
    uint32_t magic; /**< Kernel object identifier */
    enum ObObjectType type; /**< Object type */
    bool extended; /**< Additional memory was allocated for this object */
    KeMutex mutex; /**< Mutex for object locking */
    struct KeProcessControlBlock *owner; /**< Object owner process, NULL if is an unassociated, kernel-wide object */
};

/**
 * @brief Object definition to be put at the very beginning of a structure
 */
#define OBJECT struct ObObjectHeader _object

/**
 * @brief Create unassociated kernel object with additional bytes allocated
 * @param type Object type
 * @param additional Additional size in bytes
 * @return Created zero-initialized object pointer or NULL on failure
 * @note This function allocates object on a heap rather than using the slab allocator
 */
void *ObCreateKernelObjectEx(enum ObObjectType type, size_t additional);

/**
 * @brief Create unassociated kernel object
 * @param type Object type
 * @return Created zero-initialized object pointer or NULL on failure
 */
void *ObCreateKernelObject(enum ObObjectType type);

/**
 * @brief Create process-associated object with additional bytes allocated
 * @param type Object type
 * @param additional Additional size in bytes
 * @return Created zero-initialized object pointer or NULL on failure
 * @note This function allocates object on a heap rather than using the slab allocator
 */
void *ObCreateObjectEx(enum ObObjectType type, size_t additional);

/**
 * @brief Create process-associated object
 * @param type Object type
 * @return Created zero-initialized object pointer or NULL on failure
 */
void *ObCreateObject(enum ObObjectType type);

/**
 * @brief Move object ownership to a new process
 * @param *object Object pointer
 * @param *pcb New owner
 * @return Status code
 */
STATUS ObChangeObjectOwner(void *object, struct KeProcessControlBlock *pcb);

/**
 * @brief Destroy kernel object
 * @param *object Object pointer
 */
void ObDestroyObject(void *object);

/**
 * @brief Lock object
 * @param *object Object to lock
 * @attention This is a yielding lock
 * @warning This function uses mutexes, which are probably reentrant, thus, this function is probably reentrant, too
 */
void ObLockObject(void *object);

/**
 * @brief Unlock object
 * @param *object Object to unlock
 */
void ObUnlockObject(void *object);


#ifdef __cplusplus
}
#endif

#endif