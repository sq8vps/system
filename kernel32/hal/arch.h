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

struct KeTaskControlBlock;

/**
 * @addtogroup arch
 * @{
*/

/**
 * @brief Enable architecture-specific IRQ
 * @param input IRQ number
 * @return Status code
 */
INTERNAL STATUS I686IrqEnable(uint32_t input);

/**
 * @brief Disable architecture-specific IRQ
 * @param input IRQ number
 * @return Status code
 */
INTERNAL STATUS I686IrqDisable(uint32_t input);

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
INTERNAL STATUS HalCreateProcessRaw(const char *name, const char *path, PrivilegeLevel pl, 
    void (*entry)(void*), void *entryContext, struct KeTaskControlBlock **tcb);

/**
 * @brief Initialize architecture-dependent scheduler part
 * @note This function panics on failure
 */
INTERNAL void HalInitializeScheduler(void);

/**
 * @brief Perform task switch immediately if new task is available
 * @attention Do not use this function. KeSchedule() should be used first to prepare new task.
*/
INTERNAL void HalPerformTaskSwitch(void);

/**
 * @brief Initialize virtual allocator
*/
INTERNAL void HalInitVirtualAllocator(void);

EXPORT
/**
 * @brief Get page table entry flags
 * @param vAddress Input virtual address
 * @param *flags Output flags
 * @return Status code
*/
EXTERN STATUS HalGetPageFlags(uintptr_t vAddress, MmMemoryFlags *flags);

EXPORT
/**
 * @brief Get current process physical address corresponding to the virtual address
 * @param vAddress Input virtual address
 * @param *pAddress Output physical address
 * @return Error code
 */
EXTERN STATUS HalGetPhysicalAddress(uintptr_t vAddress, uintptr_t *pAddress);

EXPORT
/**
 * @brief Map current process memory
 * @param vAddress Address to map the memory to
 * @param pAddress Physical memory address
 * @param flags Page flags
 * @return Error code
 * @attention This function does not allocate physical memory.
*/
EXTERN STATUS HalMapMemory(uintptr_t vAddress, uintptr_t pAddress, MmMemoryFlags flags);

EXPORT
/**
 * @brief Map multiple pages of current process memory
 * @param vAddress Address to map the memory to
 * @param pAddress Physical memory address
 * @param size Memory size in bytes
 * @param flags Page flags
 * @return Error code
 * @attention This function does not allocate physical memory.
*/
EXTERN STATUS HalMapMemoryEx(uintptr_t vAddress, uintptr_t pAddress, uintptr_t size, MmMemoryFlags flags);

EXPORT
/**
 * @brief Unmap current process memory
 * @param vAddress Address to map the memory to
 * @return Error code
 * @attention This function does not free physical memory.
*/
EXTERN STATUS HalUnmapMemory(uintptr_t vAddress);

EXPORT
/**
 * @brief Unmap multiple pages of current process memory
 * @param vAddress Address to unmap
 * @param size Memory size in bytes
 * @return Error code
 * @attention This function does not free physical memory
*/
EXTERN STATUS HalUnmapMemoryEx(uintptr_t vAddress, uintptr_t size);

/**
 * @}
*/

#endif