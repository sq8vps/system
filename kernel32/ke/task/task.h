#ifndef KERNEL_TASK_H_
#define KERNEL_TASK_H_

#include <stdint.h>
#include "defines.h"
#include "ob/ob.h"
#include "hal/archdefs.h"
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
 * @brief Task type
 */
enum KeTaskType
{
    KE_TASK_TYPE_PROCESS, /**< Process, that is, the very first task with given memory space */
    KE_TASK_TYPE_THREAD, /**< Thread, that is, a child task created for given parent process */
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

/**
 * @brief A structure storing all task data
*/
struct KeTaskControlBlock
{
    struct ObObjectHeader objectHeader; /**< Object manager header */

    enum KeTaskType type; /**< Task type */
    uint32_t flags; /**< Task flags */

    struct HalCpuState cpu; /**< Architecture-specific CPU context */
    void *mathState; /**< Architecture-specific FPU context */

    uintptr_t userStackSize; /**< Current user mode stack size */
    void *userStackTop; /**< Initial user mode stack top */
    uintptr_t kernelStackSize; /**< Current kernel mode stack size */
    void *kernelStackTop; /**< Initial kernel mode stack top */
    void *heapTop; /**< Current user mode heap top */
    uintptr_t heapSize; /**< Current user mode heap size */
    HalCpuBitmap affinity; /**< CPU affinity */

    void *image; /**< Image start address */
    uintptr_t imageSize; /**< Image size */
    
    PrivilegeLevel pl; /**< Task privilege level */

    char name[TCB_MAX_NAME_LENGTH + 1]; /**< Task name */
    char *path; /**< Task image path */

    int taskCount; /**< Number of tasks associated with this process */
    int freeTaskIds[MAX_KERNEL_MODE_THREADS - 1]; /**< Free local IDs for children tasks */
    int threadId; /**< Thread ID within a task, starting from 1 */

    struct
    {
        struct IoFileHandle *fileList;
        uint32_t openFileCount;
    } files;
    
    KE_TASK_ID tid; //unique task ID
    union
    {
        struct KeTaskControlBlock *child;
        struct KeTaskControlBlock *parent; //parent process of this thread
    };
    struct KeTaskControlBlock *sibling;

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
};

/**
 * @brief Free TCB-related structures and destroy TCB
 * @param *tcb TCB to be destroyed
 */
void KeDestroyTCB(struct KeTaskControlBlock *tcb);

/**
 * @brief Allocate and prepare task control block
 * @param pl Privilege level
 * @param *name Task name
 * @param *path Task image path - might be NULL
 * @return Task control block pointer or NULL on memory allocation failure
 * @warning This function is used internally. Use KeCreateProcess() to create process.
*/
struct KeTaskControlBlock* KePrepareTCB(PrivilegeLevel pl, const char *name, const char *path);


/**
 * @brief Create kernel mode process
 * @param *name Process name
 * @param *entry Process entry point, must be within the kernel space
 * @param *entryContext Entry point parameter
 * @param **tcb Output Task Control Block
 * @return Status code
 * @attention This function returns immidiately
 * @attention Created task must be enabled with \a KeEnableTask() before it can be executed
*/
STATUS KeCreateKernelProcess(const char *name, void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb);

/**
 * @brief Create user mode process
 * @param *name Process name
 * @param *path Program image path
 * @param **tcb Output Task Control Block
 * @return Status code
 * @attention This function returns immidiately
 * @attention Created task must be enabled with \a KeEnableTask() before it can be executed
*/
STATUS KeCreateUserProcess(const char *name, const char *path, struct KeTaskControlBlock **tcb);

/**
 * @brief Create thread within the given process
 * @param *parent Parent process TCB
 * @param *name Thread name
 * @param *entry Thread entry point
 * @param *entryContext Entry point parameter
 * @param **tcb Output Task Control Block
 * @return Status code
 * @attention This function returns immidiately. The created thread will be started by the scheduler later.
 * @note This function can be called by any task on behalf of any task
*/
STATUS KeCreateThread(struct KeTaskControlBlock *parent, const char *name,
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb);

END_EXPORT_API

#endif