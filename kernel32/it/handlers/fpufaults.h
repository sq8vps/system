#ifndef KERNEL_FPUFAULTS_H_
#define KERNEL_FPUFAULTS_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/core/panic.h"
#include "defines.h"

INTERNAL IT_HANDLER void ItFpuUnavailableHandler(struct ItFrame *f)
{
    //TODO: implement FPU handling
    BP();
    while(1);
}

INTERNAL IT_HANDLER void ItSimdFpuHandler(struct ItFrame *f)
{
    //TODO: implement FPU handling
    BP();
    while(1);
}


#endif