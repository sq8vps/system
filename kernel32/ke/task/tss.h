#ifndef KERNEL_TSS_H_
#define KERNEL_TSS_H_

#include <stdint.h>
#include "defines.h"

STATUS KePrepareTSS(uint16_t cpu);

void KeUpdateTSS(uint16_t cpu, uintptr_t esp0);

#endif