#include "hal.h"
#include "pic.h"
#include "../it/it.h"

uint8_t HalIOPortReadByte(uint16_t port)
{
   uint8_t r = 0;
   asm("in al, dx" : "=a" (r) : "d" (port));
   return r;
}

void HalIOPortWriteByte(uint16_t port, uint8_t d)
{
    asm("out dx, al" : :"a" (d), "d" (port));
}

uint16_t HalIOPortReadWord(uint16_t port)
{
   uint16_t r = 0;
   asm("in ax, dx" : "=a" (r) : "d" (port));
   return r;
}

void HalIOPortWriteWord(uint16_t port, uint16_t d)
{
    asm("out dx, ax" : : "a" (d), "d" (port));
}

uint32_t HalIOPortReadDWord(uint16_t port)
{
   uint32_t r = 0;
   asm("in eax, dx" : "=a" (r) : "d" (port));
   return r;
}

void HalIOPortWriteDWord(uint16_t port, uint32_t d)
{
    asm("out dx, eax" : :"a" (d), "d" (port));
}

void HalInitInterruptController(void)
{
    PicSetIRQMask(0xFFFF); //mask all interrupts
    PicRemap(IT_FIRST_INTERRUPT_VECTOR, IT_FIRST_INTERRUPT_VECTOR + 8); //remap PIC interrupts to start at 0x20 (32)
}

STATUS HalEnableInterrupt(uint8_t irq)
{
    if(irq < IT_FIRST_INTERRUPT_VECTOR)
        return IT_BAD_VECTOR;
    
    return PicEnableIRQ(irq - IT_FIRST_INTERRUPT_VECTOR);
}

STATUS HalClearInterruptFlag(uint8_t irq)
{
    if(irq < IT_FIRST_INTERRUPT_VECTOR)
        return IT_BAD_VECTOR;
    return PicSendEOI(irq - IT_FIRST_INTERRUPT_VECTOR);
}