#if defined(__i686__) || defined(__amd64__)

#include "fpu.h"
#include "mm/heap.h"
#include "common.h"

#define FPU_STATE_BUFFER_SIZE 108
static char FpuDefaultState[FPU_STATE_BUFFER_SIZE];

void FpuInit(void)
{
    uint32_t cr0;
    ASM("mov eax,cr0" : "=a" (cr0) : :);
    cr0 &= ~((1 << 2) | (1 << 3)); //clear emulation bit and task switched bit
    cr0 |= (1 << 1) | (1 << 4) | (1 << 5); //set monitor co-processor bit, 80387 bit and FPU exception enable bit
    ASM("mov cr0,eax" : : "a" (cr0) :);
    ASM("fninit");
    FpuStore(FpuDefaultState);
}

void *FpuCreateStateBuffer(void)
{
    void *ret = MmAllocateKernelHeapAligned(FPU_STATE_BUFFER_SIZE, 16);
    if(NULL != ret)
        CmMemcpy(ret, FpuDefaultState, FPU_STATE_BUFFER_SIZE);
    return ret;
}

void FpuDestroyStateBuffer(const void *math)
{
    MmFreeKernelHeap(math);
}

void FpuHandleException(void)
{
    ASM("fnclex"); //clear exception flags
    uint16_t cw;
    ASM("fnstcw [%0]" : "=m"(cw) :); //read original control word
    cw &= ~(0x3F); //mask exceptions
    ASM("fldcw [%0]" : : "m"(cw) : ); //write control word
    //let the FPU handle the exception by itself
}

void FpuStore(void *buffer)
{
    ASM("fsave [eax]" : : "a" (buffer) : );
}

void FpuRestore(void *buffer)
{
    ASM("frstor [eax]" : : "a" (buffer) : );
}

#endif