#ifndef KERNEL_NMI_H_
#define KERNEL_NMI_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/panic.h"
#include "defines.h"

IT_HANDLER void ItNmiHandler(struct ItFrame *f)
{
    KePanicFromInterrupt(NULL, f->ip, NON_MASKABLE_INTERRUPT);
    while(1);
}

#endif