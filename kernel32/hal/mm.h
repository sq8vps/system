#ifndef HAL_MM_H_
#define HAL_MM_H_

#include "defines.h"
#include <stdint.h>
#include "mm/mm.h"

EXPORT_API

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

END_EXPORT_API

/**
 * @brief Get base address of driver memory space
 * @return Space base address
 */
INTERNAL uintptr_t HalGetDriverSpaceBase(void);

/**
 * @brief Get base address of dynamic memory space
 * @return Space base address
 */
INTERNAL uintptr_t HalGetDynamicSpaceBase(void);

/**
 * @brief Get base address of heap memory space
 * @return Space base address
 */
INTERNAL uintptr_t HalGetHeapSpaceBase(void);

/**
 * @brief Get size of driver memory space
 * @return Space size
 */
INTERNAL uintptr_t HalGetDriverSpaceSize(void);

/**
 * @brief Get size of dynamic memory space
 * @return Space size
 */
INTERNAL uintptr_t HalGetDynamicSpaceSize(void);

/**
 * @brief Get size of heap memory space
 * @return Space size
 */
INTERNAL uintptr_t HalGetHeapSpaceSize(void);

#endif


