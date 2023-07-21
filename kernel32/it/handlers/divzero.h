#ifndef KERNEL_DIVZERO_H_
#define KERNEL_DIVZERO_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/panic.h"
#include "defines.h"

IT_HANDLER void ItDivisionByZeroHandler(struct ItFrame *f)
{
    if(ItIsCausedByKernelMode(f->cs))
    {
        //find module
        KePanicFromInterrupt(NULL, f->ip, DIVISION_BY_ZERO);
        while(1);
    }
    else
    {
        //terminate task and switch to another task
        KePanicFromInterrupt(NULL, f->ip, DIVISION_BY_ZERO);
        while(1);
    }
}

#endif