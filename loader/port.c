#include "port.h"

//odczytuje bajt z portu
uint8_t port_readByte(uint16_t port)
{
   uint8_t r = 0;
   asm volatile("in al,dx" : "=a" (r) : "d" (port));
   return r;
}

//wpisuje bajt na port
void port_writeByte(uint16_t port, uint8_t d)
{
    asm volatile("out dx,al" : :"a" (d), "d" (port));
}

//odczytuje slowo z portu
uint16_t port_readWord(uint16_t port)
{
   uint16_t r = 0;
   asm volatile("in ax,dx" : "=a" (r) : "d" (port));
   return r;
}

//wpisuje slowo na port
void port_writeWord(uint16_t port, uint16_t d)
{
    asm volatile("out dx,ax" : :"a" (d), "d" (port));
}


uint32_t port_readDword(uint16_t port)
{
   uint32_t r = 0;
   asm volatile("in eax,dx" : "=a" (r) : "d" (port));
   return r;
}


void port_writeDword(uint16_t port, uint32_t d)
{
    asm volatile("out dx,eax" : :"a" (d), "d" (port));
}
