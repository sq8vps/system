#ifndef KERNEL_DFAULT_H_
#define KERNEL_DFAULT_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/core/panic.h"
#include "defines.h"

INTERNAL IT_HANDLER void ItDoubleFaultHandler(struct ItFrameEC *f)
{
    KePanicFromInterrupt(NULL, f->ip, DOUBLE_FAULT);
    while(1);
}

INTERNAL IT_HANDLER void ItMachineCheckHandler(struct ItFrame *f)
{
    KePanicFromInterrupt(NULL, f->ip, MACHINE_CHECK_FAULT);
    while(1)
    ;
}

#endif