#ifndef KERNEL_UD_H_
#define KERNEL_UD_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/core/panic.h"
#include "defines.h"

INTERNAL IT_HANDLER void ItInvalidOpcodeHandler(struct ItFrame *f)
{
    if(ItIsCausedByKernelMode(f->cs))
    {
        KePanicIP(f->ip, INVALID_OPCODE);
    }
    else
    {
        //TODO: terminate task and switch to another task
        KePanicIP(f->ip, INVALID_OPCODE);
    }
}

#endif