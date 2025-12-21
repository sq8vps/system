#ifndef I686_IPI_H_
#define I686_IPI_H_

#include "defines.h"
#include <stdint.h>
#include <stdbool.h>
#include "hal/cpu.h"

enum I686IpiType
{
    I686_IPI_TLB_SHOOTDOWN,
    I686_IPI_CPU_SHUTDOWN,
    I686_IPI_FUNCTION_CALL,
};

typedef int (*I686RemoteFunction)(void *context);

struct I686IpiData
{
    enum I686IpiType type; /**< IPI type, used to determine payload type */
    uint16_t source; /**< Source CPU number */
    volatile uint16_t * volatile remainingAcks; /**< Remaining acknowledges, must be atomically decremented by each recipient */
    union
    {
        struct
        {
            uintptr_t address;
            uintptr_t count;
            bool kernel;
            uintptr_t cr3;
        } tlb;
        struct
        {
            I686RemoteFunction function;
            void *context;
            int *result;
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
INTERNAL void I686SendInvalidateTlb(const HalCpuBitmap *targets, uintptr_t cr3, uintptr_t address, uintptr_t pages);

/**
 * @brief Invalidate TLB on all CPUs (kernel pages)
 * @param address Starting address to be invalidated
 * @param pages Count of pages to be invalidated
 */
INTERNAL void I686SendInvalidateKernelTlb(uintptr_t address, uintptr_t pages);

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

#endif