#ifndef KERNEL_FPUFAULTS_H_
#define KERNEL_FPUFAULTS_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/core/panic.h"
#include "defines.h"
#include "sdrv/fpu.h"

INTERNAL IT_HANDLER void ItSimdFpuHandler(struct ItFrame *f)
{
    //TODO: implement FPU handling
    while(1);
}

INTERNAL IT_HANDLER void ItFpuHandler(struct ItFrame *f)
{
    FpuHandleException();
}

#endif