#ifndef KERNEL_DFAULT_H_
#define KERNEL_DFAULT_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/panic.h"
#include "defines.h"

IT_HANDLER void ItDoubleFaultHandler(struct ItFrameEC *f)
{
    KePanicFromInterrupt(NULL, f->ip, DOUBLE_FAULT);
    while(1);
}

IT_HANDLER void ItMachineCheckHandler(struct ItFrame *f)
{
    KePanicFromInterrupt(NULL, f->ip, MACHINE_CHECK_FAULT);
    while(1)
    ;
}

#endif