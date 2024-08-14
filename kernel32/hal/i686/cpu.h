#ifndef I686_CPU_H_
#define I686_CPU_H_

#include "defines.h"
#include <stdbool.h>

/**
 * @brief Configure bootstrap CPU
 * @return Status code
 */
INTERNAL STATUS I686ConfigureBootstrapCpu(void);

/**
 * @brief Start application CPUs in MP environment
 * @return Status code
 */
INTERNAL STATUS I686StartProcessors(void);

#endif