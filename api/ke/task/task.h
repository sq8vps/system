//This header file is generated automatically
#ifndef EXPORTED___API__KE_TASK_TASK_H_
#define EXPORTED___API__KE_TASK_TASK_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "ob/ob.h"
struct ObObjectHeader;
struct _KeSemaphore;
struct _KeMutex;
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
    TASK_TERMINATED, //task was terminated and should be removed completely
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
    TASK_BLOCK_SLEEP,
    TASK_BLOCK_MUTEX,
    TASK_BLOCK_EVENT,
};

#define TCB_EFLAGS_IF (1 << 9) //interrupt flag
#define TCB_EFLAGS_RESERVED (1 << 1) //reserved EFLAGS bits
#define TCB_EFLAGS_IOPL_USER (3 << 12) //user mode EFLAGS bits

#define TCB_MAX_NAME_LENGTH (64) //max task name lanegth
#define TCB_MINOR_PRIORITY_LIMIT (15) //task priority limit

#define TCB_DEFAULT_MAJOR_PRIORITY (PRIORITY_NORMAL) //task default scheduling policy
#define TCB_DEFAULT_MINOR_PRIORITY (7) //task default priority

/**
 * @brief A structure storing all task data
*/
struct KeTaskControlBlock
{
    struct ObObjectHeader objectHeader;

    struct
    {
        uintptr_t esp; //stack pointer
        uintptr_t esp0; //kernel stack pointer for privilege level change
        uintptr_t cr3; //task page directory address
        uint16_t ds; //task data segment register
        uint16_t es; //task extra segment register
        uint16_t fs; //task extra segment register
        uint16_t gs; //task extra segment register
    } cpu;

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

    struct _KeMutex *mutex;
    struct _KeSemaphore *semaphore;
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
    struct KeTaskControlBlock **queue; //queue top handle
} PACKED;

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
extern STATUS KeCreateProcessRaw(const char *name, const char *path, PrivilegeLevel_t pl, 
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb);

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
extern STATUS KeCreateProcess(const char *name, const char *path, PrivilegeLevel_t pl, struct KeTaskControlBlock **tcb);


#ifdef __cplusplus
}
#endif

#endif