#ifndef HAL_TASK_H_
#define HAL_TASK_H_

#include "defines.h"
#include <stdint.h>

struct KeTaskControlBlock;
struct KeProcessControlBlock;

/**
 * @brief Create process
 * @param *name Process name
 * @param *path Process image path
 * @param pl Process privilege level
 * @param flags Main task flags
 * @param *entry Program entry point
 * @param *entryContext Entry point parameter
 * @param **tcb Output Task Control Block
 * @return Status code
 * @warning This function is for internal use only. User \a KeCreateKernelProcess() or \a KeCreateUserProcess()
 * @attention This function returns immidiately
*/
INTERNAL STATUS HalCreateProcess(const char *name, const char *path, PrivilegeLevel pl, uint32_t flags, 
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb);

/**
 * @brief Create thread within the given process
 * @param *parent Parent PCB
 * @param *name Thread name
 * @param flags Main task flags
 * @param *entry Thread entry point
 * @param *entryContext Entry point parameter
 * @param *userStack User mode stack top pointer
 * @param **tcb Output Task Control Block
 * @return Status code
 * @attention This function returns immidiately. The created thread will be started by the scheduler later.
*/
INTERNAL STATUS HalCreateThread(struct KeProcessControlBlock *pcb, const char *name, uint32_t flags,
    void (*entry)(void*), void *entryContext, void *userStack, struct KeTaskControlBlock **tcb);

/**
 * @brief Initialize architecture-dependent scheduler part
 * @note This function panics on failure
 */
INTERNAL void HalInitializeScheduler(void);

/**
 * @brief Perform task switch immediately if new task is available
 * @attention Do not use this function. The only legal function to force task switch is \a KeTaskYield()
*/
INTERNAL void HalPerformTaskSwitch(void);

#endif