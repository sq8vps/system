/**
 * @file i686.h
 * @brief i686-specific general definitions and types
 * @ingroup i686
 */

#ifndef I686_H_
#define I686_H_


#include <stdint.h>
#include "defines.h"
#include <stdbool.h>

/**
 * @addtogroup i686 i686 HAL implementation and architecture-specific stuff
 * @{
 */

DRIVER_API

struct KeSpinlock;

#if defined(__i686__) || defined(__amd64__)

/**
 * @brief Page size
 */
#define PAGE_SIZE 4096


/**
 * @brief Physical address data type
 */
typedef uint64_t PADDRESS;
/**
 * @brief Physical memory size data type
 */
typedef uint64_t PSIZE;

/**
 * @brief Number of defined physical memory pools
 */
#define HAL_PHYSICAL_MEMORY_POOLS 3

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
 * @brief IRQ delivery modes
 * 
 * While there is no abstraction provided over IRQ delivery modes, the IRQs are very architecture-dependent.
 * Moreover, the IRQs are used only by the drivers, and drivers are also hardware-specific and should be aware
 * of these modes. The ones provided here are APIC IRQ modes on x86.
*/
enum HalInterruptMode
{
    HAL_IT_MODE_FIXED = 0,
    HAL_IT_MODE_LOWEST_PRIORITY = 1,
    HAL_IT_MODE_SMI = 2,
    HAL_IT_MODE_NMI = 3,
    HAL_IT_MODE_INIT = 4,
    HAL_IT_MODE_EXTINT = 5,
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
 * @brief Root device ID
 */
#define HAL_ROOT_DEVICE_ID "ACPI"

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

/**
 * @brief i686-specific task context - registers
 */
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

/**
 * @brief i686-specific process data
 */
struct HalProcessData
{
    uint32_t cr3; /**< Process page directory */
    struct KeSpinlock *userMemoryLock; /**< Spinlock for modyfing process memory */
} PACKED;
#endif

/**
 * @brief i686=specific CPU object extensions
 */
struct HalCpuExtensions
{
    uint8_t lapicId; /**< LAPIC ID */
    bool bootstrap; /**< Is this CPU a bootstrap one? */
};

/**
 * @brief Lower (real mode) memory size
 */
#define I686_LOWER_MEMORY_SIZE (uintptr_t)0x100000

/**
 * @brief Virtual memory space size
 */
#define HAL_VIRTUAL_SPACE_SIZE (((uint64_t)1 << 32))

/**
 * @brief Kernel image address
 */
#define HAL_KERNEL_IMAGE_ADDRESS (uintptr_t)0xD6000000

/**
 * @brief Kernel space base address
 */
#define HAL_KERNEL_SPACE_BASE (uintptr_t)0xD0000000

/**
 * @brief Kernel space size
 */
#define HAL_KERNEL_SPACE_SIZE (HAL_VIRTUAL_SPACE_SIZE - HAL_KERNEL_SPACE_BASE)

/**
 * @brief User space top
 */
#define HAL_USER_SPACE_TOP (HAL_KERNEL_SPACE_BASE - PAGE_SIZE)

/**
 * @brief Video output is assumed to be available on PCs (VGA)
 */
#define HAL_VIDEO_AVAILABLE 1

/**
 * @brief Native register-sized type
 */
typedef uint32_t reg_t;

END_DRIVER_API

#if defined(__i686__)
    #if defined(PAE)
        /**
         * @brief Architecture name string
         */
        #define HAL_ARCHITRECTURE_STRING "i686-pae"
    #else
        /**
         * @brief Architecture name string
         */
        #define HAL_ARCHITRECTURE_STRING "i686"
    #endif
#endif

/**
 * @}
 */

#endif