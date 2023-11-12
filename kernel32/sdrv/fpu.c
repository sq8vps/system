#include "fpu.h"

void FpuInitialize(void)
{
    uint32_t cr0;
    ASM("mov eax,cr0" : "=a" (cr0) : :);
    cr0 &= ~((1 << 2) | (1 << 3)); //clear emulation bit and task switched bit
    cr0 |= (1 << 1) | (1 << 4) | (1 << 5); //set monitor co-processor bit, 80387 bit and FPU exception enable bit
    ASM("mov cr0,eax" : : "a" (cr0) :);
    ASM("fninit");
}