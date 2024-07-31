#if defined(__i686__) || defined(__amd64__)

#include "sse.h"
#include "mm/heap.h"
#include "common.h"

#define SSE_STATE_BUFFER_SIZE 512
#define SSE_STATE_BUFFER_ALIGNMENT 16
static char SseDefaultState[SSE_STATE_BUFFER_SIZE] ALIGN(SSE_STATE_BUFFER_ALIGNMENT);

void SseInit(void)
{
    uint32_t cr4;
    ASM("mov eax,cr4" : "=a" (cr4) : :);
    cr4 |= (1 << 9) | (1 << 10); //enable SSE and unmask SSE exceptions
    ASM("mov cr4,eax" : : "a" (cr4) :);
    SseStore(SseDefaultState);
}

void *SseCreateStateBuffer(void)
{
    void *ret = MmAllocateKernelHeap(SSE_STATE_BUFFER_SIZE);
    if(NULL != ret)
        CmMemcpy(ret, SseDefaultState, SSE_STATE_BUFFER_SIZE);
    return ret;
}

void SseStore(void *buffer)
{
    ASM("fxsave [eax]" : : "a" (buffer) : );
}

void SseRestore(void *buffer)
{
    ASM("fxrstor [eax]" : : "a" (buffer) : );
}

#endif