/**
 * @file idle.h
 * @brief Idle task support
 * @ingroup ke_sched
 */

#ifndef KERNEL_IDLER_H_
#define KERNEL_IDLER_H_

#include <stdint.h>
#include "defines.h"

/**
 * @addtogroup ke_sched
 * @{
 */

/**
 * @brief Create and register idle task
 * @return Status code
 * @kinternal
*/
INTERNAL STATUS KeCreateIdleTask(void);

/**
 * @}
 */

#endif