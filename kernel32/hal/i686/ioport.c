#if defined(__i686__) || defined(__amd64__)

#include "ioport.h"
#include "common.h"

uint8_t IoPortReadByte(uint16_t port)
{
   uint8_t r = 0;
   ASM("in al, dx" : "=a" (r) : "d" (port));
   return r;
}

void IoPortWriteByte(uint16_t port, uint8_t d)
{
    ASM("out dx, al" : :"a" (d), "d" (port));
}

uint16_t IoPortReadWord(uint16_t port)
{
   uint16_t r = 0;
   ASM("in ax, dx" : "=a" (r) : "d" (port));
   return r;
}

void IoPortWriteWord(uint16_t port, uint16_t d)
{
    ASM("out dx, ax" : : "a" (d), "d" (port));
}

uint32_t IoPortReadDWord(uint16_t port)
{
   uint32_t r = 0;
   ASM("in eax, dx" : "=a" (r) : "d" (port));
   return r;
}

void IoPortWriteDWord(uint16_t port, uint32_t d)
{
    ASM("out dx, eax" : :"a" (d), "d" (port));
}

// static uint32_t HalIoPortUsage[65536 / 32];
// #define IS_IO_PORT_USED(n) (HalIoPortUsage[(n) >> 5] & ((uint32_t)1 << ((n) & 31)))
// #define MARK_IO_PORT_USED(n) (HalIoPortUsage[(n) >> 5] |= ((uint32_t)1 << ((n) & 31)))
// #define MARK_IO_PORT_FREE(n) (HalIoPortUsage[(n) >> 5] &= ~((uint32_t)1 << ((n) & 31)))

// uint16_t HalIoPortAssign(uint16_t count)
// {
//     uint16_t start = 0;
//     for(uint32_t i = 0; i < 65536; i++)
//     {
//         if(!IS_IO_PORT_USED(i))
//         {
//             start = i;
//             for(uint32_t k = 0; k <= count; k++)
//             {
//                 if(k == count)
//                 {
//                     for(uint16_t m = 0; m < count; m++)
//                         MARK_IO_PORT_USED(start + m);
//                     return start;
//                 }
//                 if(IS_IO_PORT_USED(i + k))
//                 {
//                     start = 0;
//                     i += k;
//                     break;
//                 }
//             }
//         }
//     }
//     return 0;
// }

// void HalIoPortFree(uint16_t first, uint16_t count)
// {
//     while(count--)
//     {
//         MARK_IO_PORT_FREE(first + count - 1);
//     }
// }

// static void HalIoPortReserve(uint16_t first, uint16_t count)
// {
//     while(count--)
//     {
//         MARK_IO_PORT_USED(first + count - 1);
//     }
// }

// void HalIoPortInit(void)
// {
//     CmMemset(HalIoPortUsage, 0, sizeof(HalIoPortUsage));
//     MARK_IO_PORT_USED(0);

//     //reserve legacy/compatibility mode I/O ports
//     HalIoPortReserve(0, 36);
//     HalIoPortReserve(0x40, 7);
//     HalIoPortReserve(0x60, 7);
//     HalIoPortReserve(0x70, 2);
//     HalIoPortReserve(0x80, 16);
//     HalIoPortReserve(0x92, 1);
//     HalIoPortReserve(0xA0, 2);
//     HalIoPortReserve(0xC0, 32);
//     HalIoPortReserve(0xE9, 1);
//     HalIoPortReserve(0x180, 8);
//     HalIoPortReserve(0x1F0, 8);
//     HalIoPortReserve(0x278, 3);
//     HalIoPortReserve(0x2F8, 8);
//     HalIoPortReserve(0x3B0, 48);
//     HalIoPortReserve(0x3F0, 8);
//     HalIoPortReserve(0x3F8, 8);
//     HalIoPortReserve(0xCF8, 1);
//     HalIoPortReserve(0xCFC, 1);
// }

#endif