#ifndef KERNEL_PAGEFAULT_H_
#define KERNEL_PAGEFAULT_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/core/panic.h"
#include "defines.h"

INTERNAL IT_HANDLER void ItPageFaultHandler(struct ItFrame *f, uint32_t error)
{
    uintptr_t cr2;
    ASM("mov %0,cr2" : "=r" (cr2) : );
    if(ItIsCausedByKernelMode(f->cs))
    {
        KePanicIPEx(f->ip, PAGE_FAULT, cr2, error, 0, 0);
        while(1);
    }
    else
    {
        //TODO: implement this
        KePanicIPEx(f->ip, PAGE_FAULT, cr2, error, 0, 0);
        while(1);
    }
}

#endif