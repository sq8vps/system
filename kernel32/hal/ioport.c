#include "ioport.h"

uint8_t PortIoReadByte(uint16_t port)
{
   uint8_t r = 0;
   ASM("in al, dx" : "=a" (r) : "d" (port));
   return r;
}

void PortIoWriteByte(uint16_t port, uint8_t d)
{
    ASM("out dx, al" : :"a" (d), "d" (port));
}

uint16_t PortIoReadWord(uint16_t port)
{
   uint16_t r = 0;
   ASM("in ax, dx" : "=a" (r) : "d" (port));
   return r;
}

void PortIoWriteWord(uint16_t port, uint16_t d)
{
    ASM("out dx, ax" : : "a" (d), "d" (port));
}

uint32_t PortIoReadDWord(uint16_t port)
{
   uint32_t r = 0;
   ASM("in eax, dx" : "=a" (r) : "d" (port));
   return r;
}

void PortIoWriteDWord(uint16_t port, uint32_t d)
{
    ASM("out dx, eax" : :"a" (d), "d" (port));
}