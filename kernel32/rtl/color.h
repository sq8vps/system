#ifndef RTL_COLOR_H_
#define RTL_COLOR_H_

#include <stdint.h>

typedef struct 
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RtlRGB;

#define RTL_RGB_BLACK (RtlRGB){.r = 0, .g = 0, .b = 0}
#define RTL_RGB_WHITE (RtlRGB){.r = 255, .g = 255, .b = 255}
#define RTL_RGB_RED (RtlRGB){.r = 255, .g = 0, .b = 0}
#define RTL_RGB_GREEN (RtlRGB){.r = 0, .g = 255, .b = 0}
#define RTL_RGB_BLUE (RtlRGB){.r = 0, .g = 0, .b = 255}

#endif