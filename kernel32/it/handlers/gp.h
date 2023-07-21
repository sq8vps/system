#ifndef KERNEL_GP_H_
#define KERNEL_GP_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/panic.h"
#include "defines.h"

IT_HANDLER void ItGeneralProtectionHandler(struct ItFrameEC *f)
{
    if(ItIsCausedByKernelMode(f->cs))
    {
        //find module
        KePanicFromInterrupt(NULL, f->ip, GENERAL_PROTECTION_FAULT);
        while(1);
    }
    else
    {
        //terminate task and switch to another task
        KePanicFromInterrupt(NULL, f->ip, GENERAL_PROTECTION_FAULT);
        while(1);
    }
}

#endif