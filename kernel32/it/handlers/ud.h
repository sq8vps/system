#ifndef KERNEL_UD_H_
#define KERNEL_UD_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/panic.h"
#include "defines.h"

IT_HANDLER void ItInvalidOpcodeHandler(struct ItFrame *f)
{
    if(ItIsCausedByKernelMode(f->cs))
    {
        //find module
        KePanicFromInterrupt(NULL, f->ip, INVALID_OPCODE);
        while(1);
    }
    else
    {
        //terminate task and switch to another task
        KePanicFromInterrupt(NULL, f->ip, INVALID_OPCODE);
        while(1);
    }
}

#endif