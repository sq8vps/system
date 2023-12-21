#ifndef KERNEL_IDLER_H_
#define KERNEL_IDLER_H_

#include <stdint.h>
#include "defines.h"

/**
 * @brief Create and register idle task
 * @return Status code
*/
STATUS KeCreateIdleTask(void);

#endif