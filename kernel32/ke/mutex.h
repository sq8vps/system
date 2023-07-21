#ifndef KERNEL_MUTEX_H_
#define KERNEL_MUTEX_H_

#include <stdint.h>
#include "defines.h"


typedef struct KeSpinLock_t
{
    uint16_t lock;
    uint32_t eflags;
} KeSpinLock_t;

/**
 * @brief Acquire spinlock without disabling IRQ
 * @param *spinlock Spinlock structure
*/
EXPORT void KeAcquireSpinlock(KeSpinLock_t *spinlock);

/**
 * @brief Acquire spinlock and disable IRQ
 * @param *spinlock Spinlock structure
*/
EXPORT void KeAcquireSpinlockDisableIRQ(KeSpinLock_t *spinlock);

/**
 * @brief Release spinlock without changing IRQ state
 * @param *spinlock Spinlock structure
*/
EXPORT void KeReleaseSpinlock(KeSpinLock_t *spinlock);

/**
 * @brief Release spinlock and enable IRQ if possible
 * @param *spinlock Spinlock structure
*/
EXPORT void KeReleaseSpinlockEnableIRQ(KeSpinLock_t *spinlock);

#endif