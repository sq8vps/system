#include <stdint.h>
#include "defines.h"
typedef struct 
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} CmRGB;

#define CM_RGB_BLACK (CmRGB){.r = 0, .g = 0, .b = 0}
#define CM_RGB_WHITE (CmRGB){.r = 255, .g = 255, .b = 255}
#define CM_RGB_RED (CmRGB){.r = 255, .g = 0, .b = 0}
#define CM_RGB_GREEN (CmRGB){.r = 0, .g = 255, .b = 0}
#define CM_RGB_BLUE (CmRGB){.r = 0, .g = 0, .b = 255}


#ifdef __cplusplus
}
#endif

#endif