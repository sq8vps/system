#ifndef KERNEL_DIVZERO_H_
#define KERNEL_DIVZERO_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/core/panic.h"
#include "defines.h"

INTERNAL IT_HANDLER void ItDivisionByZeroHandler(struct ItFrame *f)
{
    if(ItIsCausedByKernelMode(f->cs))
    {
        //find module
        KePanicIP(f->ip, DIVISION_BY_ZERO);
        while(1);
    }
    else
    {
        //terminate task and switch to another task
        KePanicIP(f->ip, DIVISION_BY_ZERO);
        while(1);
    }
}

#endif