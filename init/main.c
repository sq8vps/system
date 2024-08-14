#include "kernel.h"

KeSpinlock s = KeSpinlockInitializer;

int main()
{
    PRIO prio = KeAcquireSpinlock(&s);
    CmPrintf("Halo\n");
    KeReleaseSpinlock(&s);
    while(1)
        ;
    return 0;
}