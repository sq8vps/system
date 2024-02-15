#ifndef CHS_H_
#define CHS_H_

#include <stdint.h>

struct Chs
{
    uint8_t head;
    uint16_t sector : 6;
    uint16_t cylinder : 10;
};

#endif