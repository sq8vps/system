#ifndef KERNEL_GP_H_
#define KERNEL_GP_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/core/panic.h"
#include "defines.h"

INTERNAL IT_HANDLER void ItGeneralProtectionHandler(struct ItFrameEC *f)
{
    if(ItIsCausedByKernelMode(f->cs))
    {
        //find module
        KePanicFromInterruptEx(NULL, f->ip, GENERAL_PROTECTION_FAULT, f->error, 0, 0, 0);
        while(1);
    }
    else
    {
        //terminate task and switch to another task
        KePanicFromInterruptEx(NULL, f->ip, GENERAL_PROTECTION_FAULT, f->error, 0, 0, 0);
        while(1);
    }
}

#endif