#ifndef KERNEL_PAGEFAULT_H_
#define KERNEL_PAGEFAULT_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/panic.h"
#include "defines.h"

IT_HANDLER void ItPAgeFaultHandler(struct ItFrameEC *f)
{
    if(ItIsCausedByKernelMode(f->cs))
    {
        //find module
        KePanicFromInterruptEx(NULL, f->ip, PAGE_FAULT, f->error, 0, 0, 0);
        while(1);
    }
    else
    {
        //TODO: implement this
        KePanicFromInterrupt(NULL, f->ip, PAGE_FAULT);
        while(1);
    }
}

#endif