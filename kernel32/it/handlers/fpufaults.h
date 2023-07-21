#ifndef KERNEL_FPUFAULTS_H_
#define KERNEL_FPUFAULTS_H_

#include <stdint.h>
#include "it/it.h"
#include "ke/panic.h"
#include "defines.h"

IT_HANDLER void ItFpuUnavailableHandler(struct ItFrame *f)
{
    //TODO: implement FPU handling
    BP();
    while(1);
}

IT_HANDLER void ItSimdFpuHandler(struct ItFrame *f)
{
    //TODO: implement FPU handling
    BP();
    while(1);
}


#endif