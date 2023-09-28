#include "kernel.h"

KeSpinlock s = KeSpinlockInitializer;

int main()
{
    KeAcquireSpinlock(&s);
    CmPrintf("Halo\n");
    KeReleaseSpinlock(&s);
    while(1)
        ;
    return 0;
}