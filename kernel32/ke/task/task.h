#ifndef KERNEL_TASK_H_
#define KERNEL_TASK_H_

#include <stdint.h>
#include "defines.h"
#include "ob/ob.h"
#include "hal/arch.h"
#include "config.h"
#include "hal/cpu.h"

EXPORT_API

struct ObObjectHeader;
struct KeSemaphore;
struct KeMutex;
struct IoFileHandle;


/**
 * @brief Task states
*/
enum KeTaskState
{
    TASK_UNINITIALIZED, //task is not initialized and won't be scheduled
    TASK_READY_TO_RUN, //task is ready to run and waiting in a queue
    TASK_RUNNING, //task is currently running
    TASK_WAITING, //task is waiting for some event
    TASK_FINISHED, //task finished and should be removed
};


/**
 * @brief Task major priority/scheduling policy
*/
enum KeTaskMajorPriority
{
    PRIORITY_HIGHEST = 0,
    PRIORITY_INTERACTIVE = 1,
    PRIORITY_NORMAL = 2,
    PRIORITY_BACKGROUND = 3,
    PRIORITY_LOWEST = 4,
};


/**
 * @brief Reason for task block (task state = TASK_WAITING)
*/
enum KeTaskBlockReason
{
    TASK_BLOCK_NOT_BLOCKED = 0,
    TASK_BLOCK_IO,
    TASK_BLOCK_EVENT_SLEEP,
    TASK_BLOCK_MUTEX,
    TASK_BLOCK_SLEEP,
};

/**
 * @brief Task flags
 */
enum KeTaskFlags
{
    KE_TASK_FLAG_IDLE = 0x01, /**< Idle task flag */
    KE_TASK_FLAG_CRITICAL = 0x02, /**< Critical task, kernel must panic if terminated */
};

/**
 * @brief Max task name length
 */
#define TCB_MAX_NAME_LENGTH (64)

#define TCB_MINOR_PRIORITY_LIMIT (15)


#define TCB_DEFAULT_MAJOR_PRIORITY (PRIORITY_NORMAL) //task default scheduling policy
#define TCB_DEFAULT_MINOR_PRIORITY (7) //task default priority

/**
 * @brief Task ID type
 */
typedef int KE_TASK_ID;

struct KeSchedulerQueue;
struct KeProcessControlBlock;

/**
 * @brief A structure storing all task data
*/
struct KeTaskControlBlock
{
    OBJECT;

    KE_TASK_ID tid; /**< Task ID */
    uint32_t flags; /**< Task flags */
    struct HalTaskData data; /**< Architecture-specific task data */
    HalCpuBitmap affinity; /**< CPU affinity */
    bool main; /**< Task is the main task in the process */

    /**
     * @brief Task stack parameters
     */
    struct
    {
        /**
         * @brief Kernel and user stack parameters
         */
        struct
        {
            void *top; /**< Stack top */
            uintptr_t size; /**< Current stack size */
        } kernel, user;
    } stack;

    struct KeProcessControlBlock *parent; /**< Task parent process */
    struct KeTaskControlBlock *sibling; /**< Next thread */

    enum KeTaskMajorPriority majorPriority; //task major scheduling priority/policy
    uint8_t minorPriority; //task minor priority

    enum KeTaskState state; //current task state
    enum KeTaskState requestedState; //next requested state
    enum KeTaskBlockReason blockReason;
    STATUS finishReason; //the reason why the task is finished
    bool notified; //task was notified to wake up

    uint64_t waitUntil; //terminal time of sleep or timeout when acquiring mutex

    // struct
    // {
    //     struct KeTaskControlBlock *attachedTo; /**< Task to which this task is attached to */
    //     struct KeTaskControlBlock *attachee; /**< Task which is attached to this task */
    //     KeMutex *mutex; /**< Mutex to be acquired in order to attach to this task */
    // } attach; /**< Task memory space attachment state */
    
    struct KeMutex *mutex;
    struct KeSemaphore *semaphore;
    struct KeRwLock *rwLock;
    struct KeTaskControlBlock *nextAux;
    struct KeTaskControlBlock *previousAux;
    
    union
    {
        struct
        {
            bool write;
        } file;

        struct
        {
            bool write;
        } rwLock;
        
    } blockParameters;
    
    struct KeTaskControlBlock *next; //next task in queue
    struct KeTaskControlBlock *previous; //previous task in queue
    struct KeSchedulerQueue *queue; //queue this task belongs to

    char name[]; /**< Task name */
};

struct KeProcessControlBlock
{
    OBJECT;
    
    uint32_t flags; /**< Process flags - currently unused */
    PrivilegeLevel pl; /**< Task privilege level */

    struct HalProcessData data; /**< Architecture-specific process data */

    /**
     * @brief Image data
     */
    struct
    {
        void *base; /**< Image base address */
        uintptr_t size; /**< Image size */
    } image;
    
    /**
     * @brief File handle container
     */
    struct
    {
        struct IoFileHandle *list; /**< File list head */
        uint32_t count; /**< Open files count */
    } files;
    
    /**
     * @brief Tasks (threads) container
     */
    struct
    {
        struct KeTaskControlBlock *list; /**< Task (thread) list */
        uint32_t count; /**< Task count */
    } tasks;

    char path[]; /**< Image path */
};


/**
 * @brief Allocate and prepare process control block
 * @param pl Privilege level
 * @param *path Process image path - might be NULL
 * @param flags Process flags
 * @return Process control block pointer or NULL on memory allocation failure
 * @warning This function is used to allocate and initialize structures and is for internal use only
*/
struct KeProcessControlBlock* KePreparePCB(PrivilegeLevel pl, const char *path, uint32_t flags);

/**
 * @brief Allocate and prepare task control block
 * @param *name Task name - must be provided
 * @param flags Task flags
 * @return Task control block pointer or NULL on memory allocation failure
 * @warning This function is used to allocate and initialize structures and is for internal use only
*/
struct KeTaskControlBlock* KePrepareTCB(const char *name, uint32_t flags);

/**
 * @brief Associate Task Control Block with given Process Control Block, i.e., register task (thread)
 * @param *pcb Target (parent) Process Control Block
 * @param *tcb Task Control Block to be associated
 * @return Status code
 */
STATUS KeAssociateTCB(struct KeProcessControlBlock *pcb, struct KeTaskControlBlock *tcb);

/**
 * @brief Dissociate Task Control Block from its parent Process Control Block
 * @param *tcb Task Control Block to be dissociated
 * @note This function does nothing if TCB is not associated
 */
void KeDissociateTCB(struct KeTaskControlBlock *tcb);

/**
 * @brief Destroy Task Control Block
 * @param *tcb Task Control Block to be destroyed
 * @note This function will dissociate the TCB from it's parent PCB
 */
void KeDestroyTCB(struct KeTaskControlBlock *tcb);

/**
 * @brief Destroy Process Control Block
 * @param *pcb Process Control Block to be destroyed
 * @warning It's the caller's responsibility to make sure that there are no remaining associated TCBs.
 */
void KeDestroyPCB(struct KeProcessControlBlock *pcb);

/**
 * @brief Create kernel mode process
 * @param *name Process name
 * @param flags Main task flags
 * @param *entry Process entry point, must be within the kernel space
 * @param *entryContext Entry point parameter
 * @param **tcb Output Task Control Block
 * @return Status code
 * @attention This function returns immediately
 * @attention Created task must be enabled with \a KeEnableTask() before it can be executed
*/
STATUS KeCreateKernelProcess(const char *name, uint32_t flags, void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb);

/**
 * @brief Create user mode process
 * @param *name Process name
 * @param *path Program image path
 * @param flags Main task flags
 * @param **tcb Output Task Control Block
 * @return Status code
 * @attention This function returns immediately
 * @attention Created task must be enabled with \a KeEnableTask() before it can be executed
*/
STATUS KeCreateUserProcess(const char *name, const char *path, uint32_t flags, struct KeTaskControlBlock **tcb);

/**
 * @brief Create thread within the given kernel mode process
 * @param *pcb Parent PCB
 * @param *name Thread name
 * @param flags Task flags
 * @param *entry Thread entry point
 * @param *entryContext Entry point parameter
 * @param **tcb Output Task Control Block
 * @return Status code
 * @attention This function returns immediately. The created thread will be started by the scheduler later.
 * @note This function can be called by any task on behalf of any kernel mode process
*/
STATUS KeCreateKernelThread(struct KeProcessControlBlock *pcb, const char *name, uint32_t flags,
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb);

/**
 * @brief Create thread within the current user mode process
 * @param *name Thread name
 * @param flags Task flags
 * @param *entry Thread entry point
 * @param *entryContext Entry point parameter
 * @param *userStack User mode stack top pointer
 * @param **tcb Output Task Control Block
 * @return Status code
 * @attention This function returns immediately. The created thread will be started by the scheduler later.
*/
STATUS KeCreateUserThread(const char *name, uint32_t flags,
    void (*entry)(void*), void *entryContext, void *userStack, struct KeTaskControlBlock **tcb);

END_EXPORT_API

#endif