#ifndef I686_ACPI_H_
#define I686_ACPI_H_

/**
 * @defgroup i686 Intel i686 architecture-dependent modules
*/

/**
 * @file acpi.h
 * @brief Minimal ACPI driver
 * 
 * A very minimalistic ACPI driver for CPU and APIC detection.
 * 
 * @defgroup acpi Minimalistic ACPI driver
 * @ingroup i686 amd64
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