/**
 * @file ipi.h
 * @brief Inter-processor interrupt module
 * @ingroup i686
 * @kinternal
 */

#ifndef I686_IPI_H_
#define I686_IPI_H_

#include "defines.h"
#include <stdint.h>
#include <stdbool.h>
#include "hal/cpu.h"

/**
 * @addtogroup i686_ipi Inter-processor interrupt module
 * @ingroup i686
 * @kinternal
 * 
 * This module handles IPIs on x86.
 * @{
 */

/**
 * @brief IPI types
 */
enum I686IpiType
{
    I686_IPI_TLB_SHOOTDOWN = 0, /**< TLB shootdown to invalidate page on another CPU */
    I686_IPI_CPU_SHUTDOWN = 1, /**< CPU shutdown */
    I686_IPI_FUNCTION_CALL = 2, /**< Remote function call */
};

/**
 * @brief Type of the remote function called using IPI
 */
typedef int (*I686RemoteFunction)(void *context);

/**
 * @brief IPI-associated data
 */
struct I686IpiData
{
    enum I686IpiType type; /**< IPI type, used to determine payload type */
    uint16_t source; /**< Source CPU number */
    volatile uint16_t * volatile remainingAcks; /**< Remaining acknowledges, must be atomically decremented by each recipient */
    union
    {
        /**
         * @brief TLB invalidation data
         */
        struct
        {
            uintptr_t address; /**< Virtual address */
            size_t count; /**< Number of pages */
            bool kernel; /**< Are these kernel pages? */
            reg_t cr3; /**< Associated CR3 (page directory) address */
        } tlb;

        /**
         * @brief Remote function call data
         */
        struct
        {
            I686RemoteFunction function; /**< Remote function pointer */
            void *context; /**< Context to pass to \a function */
            int *result; /**< Return value from \a function */
        } call;
    } payload;
};

/**
 * @brief Initialize IPI module
 * @return Status code
 */
INTERNAL STATUS I686InitializeIpi(void);

/**
 * @brief Invalidate TLB on given CPUs on which a given page directory is used
 * @param *targets Target CPU bitmap
 * @param cr3 Page directory address to be matched
 * @param address Starting address to be invalidated
 * @param pages Count of pages to be invalidated
 */
INTERNAL void I686SendInvalidateTlb(const HalCpuBitmap *targets, reg_t cr3, uintptr_t address, size_t pages);

/**
 * @brief Invalidate TLB on all CPUs (kernel pages)
 * @param address Starting address to be invalidated
 * @param pages Count of pages to be invalidated
 */
INTERNAL void I686SendInvalidateKernelTlb(uintptr_t address, size_t pages);

/**
 * @brief Send shut down command to CPUs
 */
INTERNAL void I686SendShutdownCpus(void);

/**
 * @brief Invoke function on given CPUs
 * @param *targets Target CPU bitmap
 * @param function Function to be invoked
 * @param *context Context to be passed to the function
 * @param results[] Table of function return value on each CPU
 */
INTERNAL void I686InvokeRemoteFunction(const HalCpuBitmap *targets, 
    I686RemoteFunction function, void *context, int results[MAX_CPU_COUNT]);

/**
 * @}
 */

#endif