#ifndef KERNEL_TRAPS_H_
#define KERNEL_TRAPS_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/core/panic.h"
#include "defines.h"

INTERNAL IT_HANDLER void ItDebugHandler(struct ItFrame *f)
{
    
}

INTERNAL IT_HANDLER void ItBreakpointHandler(struct ItFrame *f)
{

}

INTERNAL IT_HANDLER void ItOverflowHandler(struct ItFrame *f)
{
    
}

INTERNAL IT_HANDLER void ItUnexpectedFaultHandler(struct ItFrame *f)
{
    KePanicIP(f->ip, UNEXPECTED_FAULT);
    while(1);
}

INTERNAL IT_HANDLER void ItUnexpectedFaultHandlerEC(struct ItFrame *f, uint32_t error)
{
    KePanicIPEx(f->ip, UNEXPECTED_FAULT, error, 0, 0, 0);
    while(1);
}

#endif