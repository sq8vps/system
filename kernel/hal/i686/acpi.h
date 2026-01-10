/**
 * @file acpi.h
 * @brief Minimalistic ACPI driver
 * @ingroup i686
 */

#ifndef I686_ACPI_H_
#define I686_ACPI_H_

#include <stdint.h>
#include "defines.h"

/**
 * @addtogroup i686
 * @{
*/

EXPORT_API

/**
 * @brief Get ACPI Root Pointer
 * @return ACPI Physical Root Pointer
 */
PADDRESS I686AcpiGetRsdp(void);

END_EXPORT_API

/**
 * @brief Initialize ACPI subsystem and return LAPIC address for the bootstrap CPU
 * @param *lapicAddress Pointer where to return the LAPIC address
 * @return Status code
 */
INTERNAL STATUS AcpiInit(uintptr_t *lapicAddress);

/**
 * @}
*/

#endif