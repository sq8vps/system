#ifndef I686_H_
#define I686_H_


#include <stdint.h>
#include "defines.h"
#include <stdbool.h>

EXPORT_API

struct KeSpinlock;

#if defined(__i686__) || defined(__amd64__)


#if defined(__i686__) && defined(PAE)
    /**
     * @brief Physical address data type
     */
    typedef uint64_t PADDRESS;
#else
    /**
     * @brief Physical address data type
     */
    typedef uintptr_t PADDRESS;
#endif


/**
 * @brief Processor priority levels
 */
enum HalPriorityLevel
{
    HAL_PRIORITY_LEVEL_PASSIVE = 0, /**< Standard (non-elevated) priority level */
    HAL_PRIORITY_LEVEL_DPC = 2, /**< Deferred procedure call/dispatcher level */
    HAL_PRIORITY_LEVEL_EXCLUSIVE = 15, /**< Exclusive priority level - no IRQs */
};


/**
 * @brief Type representing task/processor priority level
*/
typedef uint8_t PRIO;


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
//vector 255 is reserved for local APIC spurious interrupt

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
struct HalCpuState
{
    uint32_t esp; //stack pointer
    uint32_t esp0; //kernel stack pointer for privilege level change
    uint32_t cr3; //task page directory address
    uint16_t ds; //task data segment register
    uint16_t es; //task extra segment register
    uint16_t fs; //task extra segment register
    uint16_t gs; //task extra segment register

    struct KeSpinlock *userPageTableLock;
} PACKED;
#endif

struct HalCpuExtensions
{
    uint8_t lapicId;
    bool bootstrap;
};

END_EXPORT_API

#endif