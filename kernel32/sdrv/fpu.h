#ifndef KERNEL_FPU_H_
#define KERNEL_FPU_H_

#include <stdint.h>
#include "defines.h"

/**
 * @brief Initialize x87 FPU
 * @attention This function assumes that the 80387+ coprocessor is available
*/
INTERNAL void FpuInitialize(void);


#endif