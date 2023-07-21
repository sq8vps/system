#ifndef KERNEL_TRAPS_H_
#define KERNEL_TRAPS_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/panic.h"
#include "defines.h"

IT_HANDLER void ItDebugHandler(struct ItFrame *f)
{
    
}

IT_HANDLER void ItBreakpointHandler(struct ItFrame *f)
{
    BP();
}

IT_HANDLER void ItOverflowHandler(struct ItFrame *f)
{
    
}

IT_HANDLER void ItUnexpectedFaultHandler(struct ItFrame *f)
{
    KePanicFromInterrupt(NULL, f->ip, UNEXPECTED_FAULT);
    while(1);
}

IT_HANDLER void ItUnexpectedFaultHandlerEC(struct ItFrameEC *f)
{
    KePanicFromInterruptEx(NULL, f->ip, UNEXPECTED_FAULT, f->error, 0, 0, 0);
    while(1);
}

#endif