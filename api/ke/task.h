//This header file is generated automatically
#ifndef API___API__KE_TASK_H_
#define API___API__KE_TASK_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "io/fs/fs.h"
/**
 * @brief Task states
*/
enum TaskState
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
enum TaskMajorPriority
{
    PRIORITY_HIGHEST = 0,
    PRIORITY_INTERACTIVE = 1,
    PRIORITY_NORMAL = 2,
    PRIORITY_BACKGROUND = 3,
    PRIORITY_LOWEST = 4,
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
    uintptr_t esp; //stack pointer
    uintptr_t esp0; //kernel stack pointer for privilege level change
    uintptr_t cr3; //task page directory address
    uint16_t ds; //task data segment register
    uint16_t es; //task extra segment register
    uint16_t fs; //task extra segment register
    uint16_t gs; //task extra segment register

    char name[TCB_MAX_NAME_LENGTH + 1]; //task name
    char *path; //task image path

    struct
    {
        struct IoFileHandle *fileList;
        uint32_t openFileCount;
    } files;
    
    uint32_t pid; //unique process ID
    uint32_t tid; //unique task ID
    struct KeTaskControlBlock *parent; //parent process for owned threads

    enum TaskMajorPriority majorPriority; //task major scheduling priority/policy
    uint8_t minorPriority; //task minor priority

    enum TaskState state; //current task state
    enum TaskState requestedState; //next requested state

    struct KeTaskControlBlock *nextWaiting; //next task waiting for some event

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