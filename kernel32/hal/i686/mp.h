#ifndef KERNEL_MP_H_
#define KERNEL_MP_H_

#include <stdint.h>
#include <stdbool.h>
#include "defines.h"


/**
 * @brief Initialize MultiProcessor module and read CPU and APIC entries.
 * @param *lapicAddress Output Local APIC physical address
 * @return Status code
 * @attention MP module should be finally deinitialized with MpDeInit()
*/
STATUS MpInit(uint32_t *lapicAddress);

// /**
//  * @brief Read and apply Local and I/O APIC entries
//  * @return Status code
//  * @warning MP module, LAPIC and IOAPIC must be initialized first.
// */
// STATUS MpGetApicInterruptEntries(void);

// /**
//  * @brief Deinitialize MP module (free memory).
// */
// void MpDeInit(void);

#endif