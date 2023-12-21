#ifndef KERNEL_ACPI_H_
#define KERNEL_ACPI_H_

/**
 * @defgroup sdrv System core drivers
*/

/**
 * @file acpi.h
 * @brief Minimal ACPI driver for APIC detection
 * 
 * A very minimalistic ACPI driver for CPU and APIC detection.
 * 
 * @defgroup acpi Minimalistic ACPI driver
 * @ingroup sdrv
*/

#include <stdint.h>
#include "defines.h"

/**
 * @addtogroup acpi
 * @{
*/
STATUS AcpiInit(uintptr_t *lapicAddress);

/**
 * @}
*/

#endif