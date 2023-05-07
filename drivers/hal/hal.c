#include "hal.h"

uint8_t Hal_IOPortReadByte(uint16_t port)
{
   uint8_t r = 0;
   asm("in al, dx" : "=a" (r) : "d" (port));
   return r;
}

void Hal_IOPortWriteByte(uint16_t port, uint8_t d)
{
    asm("out dx, al" : :"a" (d), "d" (port));
}

uint16_t Hal_IOPortReadWord(uint16_t port)
{
   uint16_t r = 0;
   asm("in ax, dx" : "=a" (r) : "d" (port));
   return r;
}

void Hal_IOPortWriteWord(uint16_t port, uint16_t d)
{
    asm("out dx, ax" : : "a" (d), "d" (port));
}

uint32_t Hal_IOPortReadDWord(uint16_t port)
{
   uint32_t r = 0;
   asm("in eax, dx" : "=a" (r) : "d" (port));
   return r;
}

void Hal_IOPortWriteDWord(uint16_t port, uint32_t d)
{
    asm("out dx, eax" : :"a" (d), "d" (port));
}