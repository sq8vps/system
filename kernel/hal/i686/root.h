/**
 * @file root.h
 * @brief Root device support
 * @ingroup i686
 */

#ifndef I686_ROOT_H_
#define I686_ROOT_H_

#include "defines.h"

/**
 * @addtogroup i686
 * @{
 */

/**
 * @brief Initialize root/core devices (ACPI, local APIC)
 * @return Status code
 */
INTERNAL STATUS I686InitRoot(void);

/**
 * @}
 */

#endif