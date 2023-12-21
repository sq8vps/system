#ifndef KERNEL_TASK_H_
#define KERNEL_TASK_H_

#include <stdint.h>
#include "defines.h"
#include "io/fs/fs.h"
#include "ke/core/mutex.h"

EXPORT
/**
 * @brief Task states
*/
enum KeTaskState
{
    TASK_UNINITIALIZED, //task is not initialized and won't be scheduled
    TASK_READY_TO_RUN, //task is ready to run and waiting in a queue
    TASK_RUNNING, //task is currently running
    TASK_WAITING, //task is waiting for some event
    TASK_TERMINATED, //task was terminated and should be removed completely
};

EXPORT
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

EXPORT
/**
 * @brief Reason for task block (task state = TASK_WAITING)
*/
enum KeTaskBlockReason
{
    TASK_BLOCK_NOT_BLOCKED = 0,
    TASK_BLOCK_IO,
    TASK_BLOCK_SLEEP,
    TASK_BLOCK_MUTEX,
    TASK_BLOCK_EVENT,
};

EXPORT
#define TCB_EFLAGS_IF (1 << 9) //interrupt flag
#define TCB_EFLAGS_RESERVED (1 << 1) //reserved EFLAGS bits
#define TCB_EFLAGS_IOPL_USER (3 << 12) //user mode EFLAGS bits

EXPORT
#define TCB_MAX_NAME_LENGTH (64) //max task name lanegth
#define TCB_MINOR_PRIORITY_LIMIT (15) //task priority limit

EXPORT
#define TCB_DEFAULT_MAJOR_PRIORITY (PRIORITY_NORMAL) //task default scheduling policy
#define TCB_DEFAULT_MINOR_PRIORITY (7) //task default priority

EXPORT
/**
 * @brief A structure storing all task data
*/
struct KeTaskControlBlock
{
    uintptr_t esp; //stack pointer
    uintptr_t esp0; //kernel stack pointer for privilege level change
    uintptr_t cr3; //task page directory address
    uint16_t ds; //task data segment register
    uint16_t es; //task extra segment register
    uint16_t fs; //task extra segment register
    uint16_t gs; //task extra segment register

    void *mathState;

    uintptr_t stackSize; //memory currently allocated for process stack
    uintptr_t heapTop;
    uintptr_t heapSize;

    PrivilegeLevel_t pl;

    char name[TCB_MAX_NAME_LENGTH + 1]; //task name
    char *path; //task image path

    struct
    {
        struct IoFileHandle *fileList;
        uint32_t openFileCount;
    } files;
    
    uint16_t pid; //unique process ID
    uint16_t tid; //unique task ID
    struct KeTaskControlBlock *parent; //parent process for owned threads

    enum KeTaskMajorPriority majorPriority; //task major scheduling priority/policy
    uint8_t minorPriority; //task minor priority

    enum KeTaskState state; //current task state
    enum KeTaskState requestedState; //next requested state
    enum KeTaskBlockReason blockReason;
    STATUS finishReason; //the reason why the task is finished

    uint64_t waitUntil; //terminal time of sleep or timeout when acquiring mutex

    union
    {
        struct _KeMutex *mutex;
        struct _KeSemaphore *semaphore;
    } timedExclusion;
    struct KeTaskControlBlock *nextAux;
    struct KeTaskControlBlock *previousAux;
    
    struct KeTaskControlBlock *next; //next task in queue
    struct KeTaskControlBlock *previous; //previous task in queue
    struct KeTaskControlBlock **queue; //queue top handle
} PACKED;


/**
 * @brief Prepare task control block
 * @param kernelStack Kernel stack top
 * @param stack Task stack top
 * @param pageDir Task page directory physical address
 * @param pl Privilege level
 * @param *name Task name
 * @param *path Task image path
 * @return Task control block pointer or NULL on memory allocation failure
 * @warning This function is used internally. Use KeCreateProcess() to create process.
*/
struct KeTaskControlBlock* KePrepareTCB(uintptr_t kernelStack, uintptr_t stack, uintptr_t pageDir, PrivilegeLevel_t pl, const char *name, const char *path);

EXPORT
/**
 * @brief Create process without default bootstrapping routine
 * @param *name Process name
 * @param *path Process image path. Not used by the kernel when using this routine.
 * @param pl Initial privilege level
 * @param *entry Program entry point
 * @param *entryContext Entry point parameter
 * @param **tcb Output Task Control Block
 * @return Status code
 * @warning Image loading and memory allocation is responsibility of caller.
 * @attention This function returns immidiately. The created process will be started by the scheduler later.
*/
EXTERN STATUS KeCreateProcessRaw(const char *name, const char *path, PrivilegeLevel_t pl, 
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb);

EXPORT
/**
 * @brief Create process
 * 
 * This function spawns process and sets the entry point to the proces bootstrap routine. The bootstrap routine
 * tries to load process image passed by "path".
 * @param *name Process name
 * @param *path Process image path
 * @param pl Privilege level
 * @param **tcb Output Task Control Block
 * @return Status code
 * @attention This function returns immidiately. The created process will be started by the scheduler later.
*/
EXTERN STATUS KeCreateProcess(const char *name, const char *path, PrivilegeLevel_t pl, struct KeTaskControlBlock **tcb);

/**
 * @brief Get highest memory available in process space
 * @return Highest memory address available (last available byte address)
*/
INTERNAL uintptr_t KeGetHighestAvailableMemory(void);

/**
 * @brief Assign task ID
 * @return Assigned task ID
*/
INTERNAL uint16_t KeAssignTid(void);

/**
 * @brief Free task ID
 * @param tid Task ID to be freed
*/
INTERNAL void KeFreeTid(uint16_t tid);

#endif