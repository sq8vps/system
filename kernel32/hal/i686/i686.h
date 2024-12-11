#ifndef I686_H_
#define I686_H_


#include <stdint.h>
#include "defines.h"
#include <stdbool.h>

EXPORT_API

struct KeSpinlock;

#if defined(__i686__) || defined(__amd64__)

/**
 * @brief Page size
 */
#define PAGE_SIZE 4096

#if defined(__i686__) && defined(PAE)
    /**
     * @brief Physical address data type
     */
    typedef uint64_t PADDRESS;
    /**
     * @brief Physical memory size data type
     */
    typedef uint64_t PSIZE;
#else
    /**
     * @brief Physical address data type
     */
    typedef uintptr_t PADDRESS;
    /**
     * @brief Physical memory size data type
     */
    typedef uintptr_t PSIZE;
#endif

/**
 * @brief Number of defined physical memory pools
 */
#define HAL_PHYSICAL_MEMORY_POOLS 2

/**
 * @brief Processor priority levels
 */
enum HalPriorityLevel
{
    HAL_PRIORITY_LEVEL_PASSIVE = 0, /**< Standard (non-elevated) priority level */
    HAL_PRIORITY_LEVEL_DPC = 2, /**< Deferred procedure call/dispatcher level */

    HAL_PRIORITY_LEVEL_IPI = 14, /**< Inter-processor interrupt level */
    HAL_PRIORITY_LEVEL_SPINLOCK = HAL_PRIORITY_LEVEL_IPI - 1, /**< Spinlock priority level */
    HAL_PRIORITY_LEVEL_EXCLUSIVE = HAL_PRIORITY_LEVEL_IPI - 1, /**< Exclusive priority level - no IRQs beside IPIs */

    HAL_PRIORITY_LEVEL_HIGHEST = 15, /**< Highest priority level - absolutely no IRQs */
};


/**
 * @brief Type representing task/processor priority level
*/
typedef uint8_t PRIO;

/**
 * @brief IRQ mode
*/
enum HalInterruptMode
{
    HAL_IT_MODE_FIXED,
    HAL_IT_MODE_LOWEST_PRIORITY,
    HAL_IT_MODE_SMI,
    HAL_IT_MODE_NMI,
    HAL_IT_MODE_INIT,
    HAL_IT_MODE_EXTINT,
};

/**
 * @brief Lowest vector available for non-kernel IRQs
*/
#define IT_IRQ_VECTOR_BASE 48


/**
 * @brief First vector available for IRQs
*/
#define IT_FIRST_INTERRUPT_VECTOR 32


/**
 * @brief Last vector available for IRQs
 */
#define IT_LAST_INTERRUPT_VECTOR 255

/**
 * @brief System timer interrupt vector
 */
#define IT_SYSTEM_TIMER_VECTOR IT_FIRST_INTERRUPT_VECTOR


/**
 * @brief Tight loop CPU "hint"
 */
#define TIGHT_LOOP_HINT() ASM("pause" : : : "memory")

/**
 * @brief Halt CPU and wait for interrupts
 */
#define HALT() ASM("hlt")

#endif


#if defined(__i686__)
struct HalTaskData
{
    uint32_t esp; //stack pointer
    uint32_t esp0; //kernel stack pointer for privilege level change
    uint32_t cr3; //task page directory address
    uint16_t ds; //task data segment register
    uint16_t es; //task extra segment register
    uint16_t fs; //task extra segment register
    uint16_t gs; //task extra segment register
    void *fpu; /**< FPU buffer */
} PACKED;

struct HalProcessData
{
    uint32_t cr3; /**< Process page directory */
    struct KeSpinlock *userMemoryLock; /**< Spinlock for modyfing process memory */
} PACKED;
#endif

struct HalCpuExtensions
{
    uint8_t lapicId;
    bool bootstrap;
};

#define I686_LOWER_MEMORY_SIZE (uintptr_t)0x100000
#define HAL_VIRTUAL_SPACE_SIZE (((uint64_t)1 << 32))

#define HAL_KERNEL_IMAGE_ADDRESS (uintptr_t)0xD6000000
#define HAL_KERNEL_SPACE_BASE (uintptr_t)0xD0000000
#define HAL_KERNEL_SPACE_SIZE (HAL_VIRTUAL_SPACE_SIZE - HAL_KERNEL_SPACE_BASE)
#define HAL_USER_SPACE_TOP (HAL_KERNEL_SPACE_BASE - PAGE_SIZE)


END_EXPORT_API

#if defined(__i686__)
    #if defined(PAE)
        #define HAL_ARCHITRECTURE_STRING "i686-pae"
    #else
        #define HAL_ARCHITRECTURE_STRING "i686"
    #endif
#endif

#endif