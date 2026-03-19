/**
 * @file task.h
 * @brief Process and thread creation and manipulation
 * @ingroup ke_task
 */

#ifndef KERNEL_TASK_H_
#define KERNEL_TASK_H_

#include "defines.h"
#include "ob/ob.h"
#include "hal/arch.h"
#include "config.h"
#include "hal/cpu.h"

/**
 * @addtogroup ke_task Process and thread support
 * @ingroup ke
 * @{
 */

DRIVER_API

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
    TASK_BLOCKED, //task is waiting for some event
    TASK_FINISHED, //task finished and should be removed
};

/**
 * @brief Scheduling policies
 */
enum KeSchedulingPolicy
{
    KE_SCHED_CFS = 0, /**< Completely Fair Scheduling-alike */
    KE_SCHED_RR = 1, /**< Prioritized round-robin */
    KE_SCHED_FCFS = 2, /** First come - first served */
    KE_SCHED_IDLE = 3, /**< Idle task scheduling */
};

/**
 * @brief Reason for task block (task state = TASK_BLOCKED)
*/
enum KeTaskBlockReason
{
    TASK_BLOCK_NOT_BLOCKED = 0, /**< Task is not blocked */
    TASK_BLOCK_IO = 1, /**< Task is waiting for an I/O operation to complete */
    TASK_BLOCK_SLEEP = 2, /**< Task is sleeping indefinetely until woken up externally */
    TASK_BLOCK_MUTEX = 3, /**< Task is waiting for a semaphore-like structure to be available */
    TASK_BLOCK_TIMED_SLEEP = 4, /**< Task requested sleep for given time and is sleeping */
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
struct MmTaskMemory;

/**
 * @brief C-style process arguments structure
 * 
 * This structure is internally used by the kernel. When \a KeCreateUserProcess() is called with non-NULL \a *argv or \a *envp provided,
 * then this structure is allocated, filled with arguments and environmental variables and passed to the newly created process.
 */
struct KeTaskArguments
{
    int argc; /**< Number of execution arguments */
    int envc; /**< Number of environmental variables */
    size_t size; /**< Data length */
    char data[]; /**< Consecutive \a argc NULL-terminated arguments and \a envc NULL-terminated environmental variables */
};

/**
 * @brief File mapping structure for creating new process
 * 
 * An array of these structures must be provided when creating user or kernel mode process to copy and map the calling process' file descriptors
 * to the new process. The last entry must be set to \a KE_TASK_FILE_MAPPING_END.
 */
struct KeTaskFileMapping
{
    int mapTo; /**< New process file handle number */
    int mapFrom; /**< Current process file handle number */
};

/**
 * @brief Last entry marker in \a KeTaskFileMapping structure
 */
#define KE_TASK_FILE_MAPPING_END {.mapTo = -1, .mapFrom = -1}

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
    void *tls; /**< Thread Local Storage pointer - set by the user mode application */

    /**
     * @brief Task stack parameters
     */
    struct
    {
        struct MmTaskMemory *user; /**< Task memory region describing user mode stack */
        /**
         * @brief Kernel stack parameters
         */
        struct
        {
            void *top; /**< Stack top */
            size_t size; /**< Current stack size */
        } kernel;
    } stack;

    struct KeProcessControlBlock *parent; /**< Task parent process */
    struct KeTaskControlBlock *sibling; /**< Next thread */

    /**
     * @brief Scheduling-related data 
     */
    struct
    {
        enum KeSchedulingPolicy policy; /**< Scheduling policy */
        int32_t priority; /**< Static scheduling priority (interpretation is policy-dependent) */
        enum KeTaskState state; //current task state
        
        bool notified; //task was notified to wake up
        
        struct KeTaskControlBlock *next; //next task in queue
        struct KeTaskControlBlock *previous; //previous task in queue

        uint64_t shadow[16]; /**< Shadowed scheduler data */
        
        uint64_t lastScheduled; /**< Timestamp of the last schedule */
        uint64_t lastRuntime; /**< Last task runtime in ns */


        /**
         * @brief Task block parameters
         */
        struct 
        {
            enum KeTaskBlockReason reason; /**< Reason for task block */
            struct KeTaskControlBlock *next; /**< Next task in semaphore-like structure queue */
            struct KeTaskControlBlock *previous; /**< Previous task in semaphore-like structure queue */
            struct KeMutex *mutex; /**< Mutex this task is waiting for */
            struct KeSemaphore *semaphore; /**< Semaphore this task is waiting for */
            struct KeRwLock *rwLock; /**< RW lock this task is waiting for */
            /**
             * @brief Data for timed wait
             */
            struct
            {
                uint64_t until; /**< Terminal timestamp */
                struct KeTaskControlBlock *next; /**< Next task in timed queue */
                struct KeTaskControlBlock *previous; /**< Previous task in timed queue */
            } timeout;
            bool acquired; /**< Set to true if semaphore-like structure acquistion was successful without timeout */
            bool write; /**< Requested state for RW lock */
            uint32_t count; /**< Requested count for semaphore */
        } block;
    

        KeSpinlock lock;
    } scheduling;



    STATUS finishReason; //the reason why the task is finished

    // struct
    // {
    //     struct KeTaskControlBlock *attachedTo; /**< Task to which this task is attached to */
    //     struct KeTaskControlBlock *attachee; /**< Task which is attached to this task */
    //     KeMutex *mutex; /**< Mutex to be acquired in order to attach to this task */
    // } attach; /**< Task memory space attachment state */
    

};

/**
 * @brief A structure storing all process (a group of at least one task) data
 */
struct KeProcessControlBlock
{
    OBJECT;
    
    uint32_t flags; /**< Process flags - currently unused */
    PrivilegeLevel pl; /**< Task privilege level */

    struct HalProcessData data; /**< Architecture-specific process data */

    /**
     * @brief Process memory region list and tree
     */
    struct
    {
        void *tree; /**< Memory region base-ordered tree root */
        struct MmTaskMemory *head; /**< Memory region base-order list head */
        struct MmTaskMemory *tail; /**< Memory region base-order list tail */
        void *base; /**< Lower mapping boundary when address is not specified */
        KeMutex mutex; /**< Memory mapping list mutex */
    } memory;
    
    /**
     * @brief File handle container
     */
    struct
    {
        struct
        {
            struct IoFileHandle *handle; /**< File handle */
            KeMutex mutex; /**< File handle access mutex */
        } *table; /**< File table */
        uint32_t tableSize; /**< Size of the file table (number of overall entries) */
        uint32_t count; /**< Open files count */
    } files;
    
    /**
     * @brief Tasks (threads) container
     */
    struct
    {
        struct KeTaskControlBlock *list; /**< Task (thread) list */
        uint32_t count; /**< Task count */
        KeSpinlock lock; /**< Task list lock */
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
 * @param flags Task flags
 * @return Task control block pointer or NULL on memory allocation failure
 * @warning This function is used to allocate and initialize structures and is for internal use only
*/
struct KeTaskControlBlock* KePrepareTCB(uint32_t flags);

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
 * @param flags Main task flags
 * @param *entry Process entry point, must be within the kernel space
 * @param *entryContext Entry point parameter
 * @param *fileMap File mapping array or NULL
 * @param **tcb Output Task Control Block
 * @return Status code
 * @attention This function returns immediately
 * @attention Created task must be enabled with \a KeEnableTask() before it can be executed
*/
STATUS KeCreateKernelProcess(uint32_t flags, void (*entry)(void*), void *entryContext, struct KeTaskFileMapping *fileMap, struct KeTaskControlBlock **tcb);

/**
 * @brief Create user mode process
 * @param *path Program image path
 * @param flags Main task flags
 * @param *argv[] Program entry arguments poiner table. Must end with a NULL pointer
 * @param *envp[] Environmental variables pointer table. Must end with a NULL pointer
 * @param *fileMap File mapping array or NULL
 * @param **tcb Output Task Control Block
 * @return Status code
 * @attention This function returns immediately
 * @attention Created task must be enabled with \a KeEnableTask() before it can be executed
*/
STATUS KeCreateUserProcess(const char *path, uint32_t flags, const char *argv[], const char *envp[], const struct KeTaskFileMapping *fileMap, struct KeTaskControlBlock **tcb);

/**
 * @brief Create thread within the given kernel mode process
 * @param *pcb Parent PCB
 * @param flags Task flags
 * @param *entry Thread entry point
 * @param *entryContext Entry point parameter
 * @param **tcb Output Task Control Block
 * @return Status code
 * @attention This function returns immediately. The created thread will be started by the scheduler later.
 * @note This function can be called by any task on behalf of any kernel mode process
*/
STATUS KeCreateKernelThread(struct KeProcessControlBlock *pcb, uint32_t flags,
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb);

/**
 * @brief Create thread within the current user mode process
 * @param flags Task flags
 * @param *entry Thread entry point
 * @param *entryContext Entry point parameter
 * @param *userStack User mode stack top pointer
 * @param **tcb Output Task Control Block
 * @return Status code
 * @attention This function returns immediately. The created thread will be started by the scheduler later.
*/
STATUS KeCreateUserThread(uint32_t flags,
    void (*entry)(void*), void *entryContext, void *userStack, struct KeTaskControlBlock **tcb);

/**
 * @brief Build task argument structure from given arguments
 * @param *argv[] Argument values pointers, with NULL being the last pointer
 * @param *envp[] Enviromental values pointers, with NULL being the last pointer
 * @return Allocated and filled task argument structure or NULL on failure
 */
struct KeTaskArguments* KeBuildTaskArguments(const char *argv[], const char *envp[]);

END_DRIVER_API

NABLA_API

/**
 * @brief Set and apply Thread Local Storage for current thread
 * @param *tls TLS to be set and applied
 * @return Always OK
 * @note No checks are performed for \a *tls, the kernel does not use it
 */
STATUS ApiSetThreadLocalStorage(void *tls);

/**
 * @brief Exit (finish) calling task
 * @param result Return code - execution result
 * @note This function does not return to the caller
 */
[[noreturn]] void ApiExitTask(int result);

END_NABLA_API

/**
 * @brief Set Thread-local Storage pointer for given task
 * @param *tcb Target Task Control Block pointer
 * @param *tls Thread-local Storage pointer
 * @note HalUpdateTls() must be used for this to take effect before a task switch
 * @kinternal
 */
INTERNAL STATUS KeSetThreadLocalStorage(struct KeTaskControlBlock *tcb, void *tls);

/**
 * @brief Destroy task (and possibly process) that was already terminated
 * 
 * This function is called by the cleanup worker after the task was terminated and detached from the scheduler.
 * It flushes files, frees task memory, and destroys all task-associated structures.
 * @param *tcb Target Task Control Block
 * @return Status code
 * @kinternal
 */
INTERNAL STATUS KeDestroyTask(struct KeTaskControlBlock *tcb);

/**
 * @}
 */

#endif