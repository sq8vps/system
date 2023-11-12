#include "sse.h"

STATUS SseInitialize(void)
{
    uint32_t cr4;
    ASM("mov eax,cr4" : "=a" (cr4) : :);
    cr4 |= (1 << 9) | (1 << 10); //enable SSE and unmask SSE exceptions
    ASM("mov cr4,eax" : : "a" (cr4) :);
    return OK;
}

STATUS SseStore(struct KeTaskControlBlock *tcb)
{
    ASM("fxsave [%0]" : "=r" (tcb->mathState) : :);
    return OK;
}

STATUS SseRestore(struct KeTaskControlBlock *tcb)
{
    ASM("fxrstor [%0]" : : "r" (tcb->mathState) :);
    return OK;
}