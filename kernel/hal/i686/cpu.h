/**
 * @file cpu.h
 * @brief CPU support
 * @ingroup i686
 * @note This moduke implements the universal HAL interface and most of its function are available using kernel API.
 */
#ifndef I686_CPU_H_
#define I686_CPU_H_

#include "defines.h"
#include <stdbool.h>

/**
 * @addtogroup i686
 * @{
 */

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

/**
 * @}
 */

#endif