#ifndef I686_ROOT_H_
#define I686_ROOT_H_

#include "defines.h"

/**
 * @brief Initialize root/core devices (ACPI/MP, local APIC)
 * @return Status code
 */
INTERNAL STATUS I686InitRoot(void);

#endif