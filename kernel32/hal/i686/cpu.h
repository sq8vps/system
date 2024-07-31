#ifndef I686_CPU_H_
#define I686_CPU_H_

#include "defines.h"
#include <stdbool.h>

/**
 * @brief Add CPU to CPU list
 */
INTERNAL void CpuAdd(uint8_t lapic, bool usable, bool bootstrap);

#endif