#ifndef KERNEL_NMI_H_
#define KERNEL_NMI_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/core/panic.h"
#include "defines.h"

INTERNAL IT_HANDLER void ItNmiHandler(struct ItFrame *f)
{
    KePanicIP(f->ip, NON_MASKABLE_INTERRUPT);
    while(1);
}

#endif