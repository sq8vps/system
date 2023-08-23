#ifndef KERNEL_PAGEFAULT_H_
#define KERNEL_PAGEFAULT_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/core/panic.h"
#include "defines.h"

INTERNAL IT_HANDLER void ItPageFaultHandler(struct ItFrameEC *f)
{
    uintptr_t cr2;
    ASM("mov %0,cr2" : "=r" (cr2) : );
    if(ItIsCausedByKernelMode(f->cs))
    {
        //find module
        KePanicFromInterruptEx(NULL, f->ip, PAGE_FAULT, cr2, f->error, 0, 0);
        while(1);
    }
    else
    {
        //TODO: implement this
        KePanicFromInterruptEx(NULL, f->ip, PAGE_FAULT, cr2, f->error, 0, 0);
        while(1);
    }
}

#endif