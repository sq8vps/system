#ifndef KERNEL32_TASK_H_
#define KERNEL32_TASK_H_

#include <stdint.h>
#include "../defines.h"

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

struct TaskControlBlock
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
    
    uint32_t pid; //unique process ID
    uint32_t tid; //task ID, identifies a thread within one process

    enum TaskMajorPriority majorPriority; //task major scheduling priority/policy
    uint8_t minorPriority; //task minor priority

    enum TaskState state; //current task state

    struct TaskControlBlock *next; //next task in queue
    struct TaskControlBlock *previous; //previous task in queue

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
struct TaskControlBlock* KePrepareTCB(uintptr_t kernelStack, uintptr_t stack, uintptr_t pageDir, PrivilegeLevel_t pl, const char *name, const char *path);

/**
 * @brief Create process without default bootstrapping routine
 * @param *name Process name
 * @param *path Process image path. Not used by the kernel when using this routine.
 * @param pl Initial privilege level
 * @param *entry Program entry point
 * @param **tcb Output Task Control Block
 * @return Status code
 * @warning Process image loading is the responsibility of caller.
 * @warning In general this function should not be used on its own.
 * @attention This function returns immidiately. The created process will be started by the scheduler later.
*/
EXPORT STATUS KeCreateProcessRaw(const char *name, const char *path, PrivilegeLevel_t pl, void (*entry)(), struct TaskControlBlock **tcb);

/**
 * @brief Create process
 * @param *name Process name
 * @param *path Process image path
 * @param pl Privilege level
 * @param **tcb Output Task Control Block
 * @return Status code
 * @attention This function returns immidiately. The created process will be started by the scheduler later.
 * 
 * This function spawns process and sets the entry point to the proces bootstrap routine. The bootstrap routine
 * tries to load process image passed by "path".
*/
EXPORT STATUS KeCreateProcess(const char *name, const char *path, PrivilegeLevel_t pl, struct TaskControlBlock **tcb);

#endif