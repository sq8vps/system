#ifndef KERNEL_GP_H_
#define KERNEL_GP_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/core/panic.h"
#include "defines.h"

INTERNAL IT_HANDLER void ItGeneralProtectionHandler(struct ItFrame *f, uint32_t error)
{
    if(ItIsCausedByKernelMode(f->cs))
    {
        KePanicIPEx(f->ip, GENERAL_PROTECTION_FAULT, error, 0, 0, 0);
        while(1);
    }
    else
    {
        //TODO: terminate task and switch to another task
        KePanicIPEx(f->ip, GENERAL_PROTECTION_FAULT, error, 0, 0, 0);
        while(1);
    }
}

#endif