#ifndef HAL_ARCH_H_
#define HAL_ARCH_H_

/**
 * @file arch.h
 * @brief Universal internal architecture-dependent routines
 * 
 * This header contains declaration of routines, which are present for every architecture,
 * but their definition is fully architecture-dependent. These routines must be implemented
 * by all architecture-specific modules.
 * 
 * @defgroup arch Universal architecture-dependent routines
 * @ingroup hal
*/

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"
#include "mm/mm.h"
#include "hal/interrupt.h"



/**
 * @addtogroup arch
 * @{
*/

EXPORT_API

struct KeTaskControlBlock;

/**
 * @brief Get page table entry flags
 * @param vAddress Input virtual address
 * @param *flags Output flags
 * @return Status code
*/
STATUS HalGetPageFlags(uintptr_t vAddress, MmMemoryFlags *flags);


/**
 * @brief Get current process physical address corresponding to the virtual address
 * @param vAddress Input virtual address
 * @param *pAddress Output physical address
 * @return Error code
 */
STATUS HalGetPhysicalAddress(uintptr_t vAddress, uintptr_t *pAddress);


/**
 * @brief Map current process memory
 * @param vAddress Address to map the memory to
 * @param pAddress Physical memory address
 * @param flags Page flags
 * @return Error code
 * @attention This function does not allocate physical memory.
*/
STATUS HalMapMemory(uintptr_t vAddress, uintptr_t pAddress, MmMemoryFlags flags);


/**
 * @brief Map multiple pages of current process memory
 * @param vAddress Address to map the memory to
 * @param pAddress Physical memory address
 * @param size Memory size in bytes
 * @param flags Page flags
 * @return Error code
 * @attention This function does not allocate physical memory.
*/
STATUS HalMapMemoryEx(uintptr_t vAddress, uintptr_t pAddress, uintptr_t size, MmMemoryFlags flags);


/**
 * @brief Unmap current process memory
 * @param vAddress Address to map the memory to
 * @return Error code
 * @attention This function does not free physical memory.
*/
STATUS HalUnmapMemory(uintptr_t vAddress);


/**
 * @brief Unmap multiple pages of current process memory
 * @param vAddress Address to unmap
 * @param size Memory size in bytes
 * @return Error code
 * @attention This function does not free physical memory
*/
STATUS HalUnmapMemoryEx(uintptr_t vAddress, uintptr_t size);

/**
 * @brief Get current CPU number
 * @return Current CPU number
 */
uint16_t HalGetCurrentCpu(void);

END_EXPORT_API

/**
 * @brief Free task structures
 * @param *tcb Target Task Control Block
 * @return Status code
 */
INTERNAL STATUS HalFreeTaskStructures(struct KeTaskControlBlock *tcb);

/**
 * @brief Register architecture-specific IRQ
 * @param input IRQ number
 * @param vector Corresponding vector
 * @param params IRQ params
 * @return Status code
 */
INTERNAL STATUS IrqRegister(uint32_t input, uint8_t vector, struct HalInterruptParams params);

/**
 * @brief Unregister architecture-specific IRQ
 * @param input IRQ number
 * @return Status code
 */
INTERNAL STATUS IrqUnregister(uint32_t input);

/**
 * @brief Obtain vector corresponding to given IRQ (if applicable)
 * @return Vector corresponding to given IRQ
 */
INTERNAL uint32_t IrqVectorFromIrq(uint32_t irq);

/**
 * @brief Check if vector number is related to IRQ due to hardware limitations
 * @return True if related, false otherwise
 */
INTERNAL bool HalIrqIsVectorRelatedToIrq(void);

/**
 * @brief Initialize architecture-specific interrupts
 * @return Status code
 */
INTERNAL STATUS ItInitializeArchitectureInterrupts(void);

/**
 * @brief Create process
 * @param *name Process name
 * @param *path Process image path
 * @param pl Process privilege level
 * @param *entry Program entry point
 * @param *entryContext Entry point parameter
 * @param **tcb Output Task Control Block
 * @return Status code
 * @warning This function is for internal use only. User \a KeCreateKernelProcess() or \a KeCreateUserProcess()
 * @attention This function returns immidiately
*/
INTERNAL STATUS HalCreateProcess(const char *name, const char *path, PrivilegeLevel pl, 
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb);

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
INTERNAL STATUS HalCreateThread(struct KeTaskControlBlock *parent, const char *name,
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb);

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

/**
 * @brief Attach kernel thread to another task's memory space
 * @param *target Target task control block
 * @return Status code
 * @attention This function is for internal use only. Use \a KeAttachToTask().
 */
INTERNAL STATUS HalAttachToTask(struct KeTaskControlBlock *target);

/**
 * @brief Enable architecture-specific IRQ
 * @param input IRQ number
 * @return Status code
 */
INTERNAL STATUS HalMasterEnableIrq(uint32_t input);

/**
 * @brief Disable architecture-specific IRQ
 * @param input IRQ number
 * @return Status code
 */
INTERNAL STATUS HalMasterDisableIrq(uint32_t input);

/**
 * @brief Halt all CPUs
 * @attention This routine is used only on failure in a SMP system
 */
INTERNAL void HalHaltAllCpus(void);

/**
 * @brief Architecture-specific initialization phase 1
 */
INTERNAL void HalInitPhase1(void);

/**
 * @brief Architecture-specific initialization phase 2
 */
INTERNAL void HalInitPhase2(void);

/**
 * @brief Architecture-specific initialization phase 3
 */
INTERNAL void HalInitPhase3(void);

/**
 * @brief Call global constructor for C++ support
 */
INTERNAL void HalCallConstructors(void);

/**
 * @}
*/

#endif