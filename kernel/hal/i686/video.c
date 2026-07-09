/*
* This module provides a very simple VGA driver for use in following cases:
* 1. During early boot stages when there are no proper display drivers loaded yet
* 2. For displaying bug check informations in panic routines
* Keep in mind that:
* 1. This driver should NOT be used to show any kernel output.
* 2. This driver should be deinitialized as soon as the proper display driver is loaded
* 3. This driver is a core system driver and does not provide a typical loadable driver interface
* 
* Currently supports 320x200 and 640x480 resolutions in graphic mode and "emulated" text mode using built-in font.
* 
* @note The resolution and color depth are hardcoded.
* @note Quite heavily based on this public domain code: https://files.osdev.org/mirrors/geezer/osd/graphics/modes.c
*/

#if defined(__i686__) || defined(__amd64__)

#include "hal/i686/ioport.h"
#include "mm/mmio.h"
#include "font.h"
#include "hal/video.h"
#include "rtl/string.h"

//uncomment to enable 640x480 mode
//otherwise 320x200 mode is used
//640x480 is slower, because it uses planar mode
#define HAL_VIDEO_USE_640_480

#ifdef HAL_VIDEO_USE_640_480
	#define HAL_VIDEO_FRAME_BUFFER_ADDRESS 0xA0000
	#define HAL_VIDEO_WIDTH 640
	#define HAL_VIDEO_HEIGHT 480
#else
	#define HAL_VIDEO_FRAME_BUFFER_ADDRESS 0xA0000
	#define HAL_VIDEO_WIDTH 320
	#define HAL_VIDEO_HEIGHT 200
#endif

#define HAL_VIDEO_AC_INDEX 0x3C0
#define HAL_VIDEO_AC_WRITE 0x3C0
#define HAL_VIDEO_AC_READ 0x3C1
#define HAL_VIDEO_MISC_WRITE 0x3C2
#define HAL_VIDEO_SEQ_INDEX 0x3C4
#define HAL_VIDEO_SEQ_DATA 0x3C5
#define HAL_VIDEO_DAC_READ_INDEX 0x3C7
#define HAL_VIDEO_DAC_WRITE_INDEX 0x3C8
#define HAL_VIDEO_DAC_DATA 0x3C9
#define HAL_VIDEO_MISC_READ 0x3CC
#define HAL_VIDEO_GC_INDEX 0x3CE
#define HAL_VIDEO_GC_DATA 0x3CF
#define HAL_VIDEO_CRTC_INDEX 0x3D4
#define HAL_VIDEO_CRTC_DATA 0x3D5
#define HAL_VIDEO_INSTAT_READ 0x3DA
#define HAL_VIDEO_NUM_SEQ_REGS 5
#define HAL_VIDEO_NUM_CRTC_REGS 25
#define HAL_VIDEO_NUM_GC_REGS 9
#define HAL_VIDEO_NUM_AC_REGS 21
#define HAL_VIDEO_NUM_REGS (1 + HAL_VIDEO_NUM_SEQ_REGS + HAL_VIDEO_NUM_CRTC_REGS + HAL_VIDEO_NUM_GC_REGS + HAL_VIDEO_NUM_AC_REGS)

static struct
{
	uint8_t bgColor;
	uint8_t fgColor;

	uint32_t x;
	uint32_t y;

	uint8_t *vmem;

	HalVideoResetRoutine reset;
	void (*resetContext)(void *context);

	bool initialized;
	bool ownership;
} HalVideoState = {.vmem = NULL, .reset = NULL, .initialized = false, .ownership = true};


#ifdef HAL_VIDEO_USE_640_480
static const uint8_t HalVideoRegs640x480x16[] =
{
/* MISC */
	0xE3,
/* SEQ */
	0x03, 0x01, 0x08, 0x00, 0x06,
/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0x0B, 0x3E,
	0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0xEA, 0x0C, 0xDF, 0x28, 0x00, 0xE7, 0x04, 0xE3,
	0xFF,
/* GC */
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x05, 0x0F,
	0xFF,
/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
	0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
	0x01, 0x00, 0x0F, 0x00, 0x00
};
#else
static const uint8_t HalVideoRegs320x200x256[] =
{
/* MISC */
	0x63,
/* SEQ */
	0x03, 0x01, 0x0F, 0x00, 0x0E,
/* CRTC */
	0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
	0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x9C, 0x0E, 0x8F, 0x28,	0x40, 0x96, 0xB9, 0xA3,
	0xFF,
/* GC */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
	0xFF,
/* AC */
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
	0x41, 0x00, 0x0F, 0x00,	0x00
};
#endif


static void HalVideoSetupVgaRegisters(const uint8_t *regs)
{
	size_t i = 0;

    /* write MISCELLANEOUS reg */
	IoPortWriteByte(HAL_VIDEO_MISC_WRITE, *regs);
	regs++;

    /* unlock CRTC registers */
	for (i = 0; i < HAL_VIDEO_NUM_SEQ_REGS; i++)
	{
		IoPortWriteByte(HAL_VIDEO_SEQ_INDEX, i);
		IoPortWriteByte(HAL_VIDEO_SEQ_DATA, *regs);
		regs++;
	}
	/* unlock CRTC registers */
	IoPortWriteByte(HAL_VIDEO_CRTC_INDEX, 0x03);
	IoPortWriteByte(HAL_VIDEO_CRTC_DATA, IoPortReadByte(HAL_VIDEO_CRTC_DATA) | 0x80);
	IoPortWriteByte(HAL_VIDEO_CRTC_INDEX, 0x11);
	IoPortWriteByte(HAL_VIDEO_CRTC_DATA, IoPortReadByte(HAL_VIDEO_CRTC_DATA) & ~0x80);
	/* write CRTC regs */
	for (i = 0; i < HAL_VIDEO_NUM_CRTC_REGS; i++)
	{
		IoPortWriteByte(HAL_VIDEO_CRTC_INDEX, i);
		IoPortWriteByte(HAL_VIDEO_CRTC_DATA, *regs);
		regs++;
	}
	/* write GRAPHICS CONTROLLER regs */
	for (i = 0; i < HAL_VIDEO_NUM_GC_REGS; i++)
	{
		IoPortWriteByte(HAL_VIDEO_GC_INDEX, i);
		IoPortWriteByte(HAL_VIDEO_GC_DATA, *regs);
		regs++;
	}
	/* write ATTRIBUTE CONTROLLER regs */
	for (i = 0; i < HAL_VIDEO_NUM_AC_REGS; i++)
	{
		IoPortReadByte(HAL_VIDEO_INSTAT_READ);
		IoPortWriteByte(HAL_VIDEO_AC_INDEX, i);
		IoPortWriteByte(HAL_VIDEO_AC_WRITE, *regs);
		regs++;
	}

	/* lock 16-color palette and unblank display */
	IoPortReadByte(HAL_VIDEO_INSTAT_READ);
	IoPortWriteByte(HAL_VIDEO_AC_INDEX, 0x20);
}

static void HalVideoWriteColorPalette(void)
{
	IoPortWriteByte(HAL_VIDEO_AC_INDEX, 0x00); //enable access to color palette
    IoPortWriteByte(HAL_VIDEO_DAC_WRITE_INDEX, 0);
    
    uint8_t r = 0, g = 0, b = 0;
#ifdef HAL_VIDEO_USE_640_480
    for(size_t i = 0; i < 16; i++)
    {
        IoPortWriteByte(HAL_VIDEO_DAC_DATA, r);
        IoPortWriteByte(HAL_VIDEO_DAC_DATA, g);
        IoPortWriteByte(HAL_VIDEO_DAC_DATA, b);

		r += 63;
		if(126 == r)
		{
			g += 63;
			r = 0;
			if(126 == g)
			{
				b += 63;
				g = 0;
			}
		}
	}
#else
    for(size_t i = 0; i < 256; i++)
    {
        IoPortWriteByte(HAL_VIDEO_DAC_DATA, r);
        IoPortWriteByte(HAL_VIDEO_DAC_DATA, g);
        IoPortWriteByte(HAL_VIDEO_DAC_DATA, b);
		//to get 64 colors/6-bit color depth (2 bits per color)
		//the step needs to be 21 as it covers full 0 to 63 range
		//the remaining 210 colors are not used and are set to some aribtrary values
        r += 21;
		if(84 == r)
		{
			g += 21;
			r = 0;
			if(84 == g)
			{
				b += 21;
				g = 0;
			}
		}
	}
#endif
	IoPortWriteByte(HAL_VIDEO_AC_INDEX, 0x20); //disable access to color palette
}

void HalRegisterVideoResetRoutine(HalVideoResetRoutine resetRoutine, void *context)
{
	HalVideoState.reset = resetRoutine;
	HalVideoState.resetContext = context;
}

STATUS HalVideoInit(void)
{	
	if(!HalVideoState.ownership)
	{
		if(NULL != HalVideoState.reset)
		{
			HalVideoState.reset(HalVideoState.resetContext);
			HalVideoState.ownership = true;
		}
		else
		{
			return DEVICE_NOT_AVAILABLE;
		}
	}

	if(NULL == HalVideoState.vmem)
	{
		HalVideoState.vmem = MmMapMmIo(HAL_VIDEO_FRAME_BUFFER_ADDRESS, HAL_VIDEO_WIDTH * HAL_VIDEO_HEIGHT);
		if(NULL == HalVideoState.vmem)
			return OUT_OF_RESOURCES;
	}

#ifdef HAL_VIDEO_USE_640_480
	HalVideoSetupVgaRegisters(HalVideoRegs640x480x16);
#else
	HalVideoSetupVgaRegisters(HalVideoRegs320x200x256);
#endif
	HalVideoWriteColorPalette();

	HalVideoState.x = 0;
	HalVideoState.y = 0;

#ifdef HAL_VIDEO_USE_640_480
	HalVideoState.bgColor = 0b000; //black
	HalVideoState.fgColor = 0b111; //white
#else
	HalVideoState.bgColor = 0; //black
	HalVideoState.fgColor = 0b111111; //white
#endif

	HalVideoState.initialized = true;

	HalVideoClearScreen();

	return OK;
}


void HalVideoDeinit(void)
{
	if(!HalVideoState.initialized)
		return;
	
	HalVideoState.initialized = false;
	HalVideoState.ownership = false;

	IoPortWriteByte(HAL_VIDEO_GC_INDEX, 0x08);
    IoPortWriteByte(HAL_VIDEO_GC_DATA, 0xff);
}

#ifdef HAL_VIDEO_USE_640_480
static inline void HalVideoSetPlane(uint8_t plane)
{
	plane &= 3;
	uint8_t mask = 1 << plane;

	IoPortWriteByte(HAL_VIDEO_GC_INDEX, 4);
	IoPortWriteByte(HAL_VIDEO_GC_DATA, plane);

	IoPortWriteByte(HAL_VIDEO_SEQ_INDEX, 2);
	IoPortWriteByte(HAL_VIDEO_SEQ_DATA, mask);
}
#endif

static inline uint8_t HalVideoNormalizeColor(RtlRGB color)
{
#ifdef HAL_VIDEO_USE_640_480
	color.r >>= 7;
	color.g >>= 7;
	color.b >>= 7;
	
	return color.r | (color.g << 1) | (color.b << 2);
#else
	color.r >>= 6;
	color.g >>= 6;
	color.b >>= 6;
	
	return color.r | (color.g << 2) | (color.b << 4);
#endif
}

void HalVideoSetBackgroundColor(RtlRGB color)
{
	HalVideoState.bgColor = HalVideoNormalizeColor(color);
}

void HalVideoSetForegroundColor(RtlRGB color)
{
	HalVideoState.fgColor = HalVideoNormalizeColor(color);
}

void HalVideoSetColor(RtlRGB fg, RtlRGB bg)
{
	HalVideoSetForegroundColor(fg);
	HalVideoSetBackgroundColor(bg);
}


HOT static inline void HalVideoSetPixelNormalized(size_t x, size_t y, uint8_t color)
{	
	if(unlikely((y >= HAL_VIDEO_HEIGHT) || (x >= HAL_VIDEO_WIDTH)))
		return;

#ifdef HAL_VIDEO_USE_640_480
	uint16_t planeMask = 1;
	uint32_t offset = HAL_VIDEO_WIDTH * y / 8 + x / 8;
	
	x &= 7;
	uint8_t mask = 0x80 >> x;
	//in this driver there are only 3 bits used in 16-color mode (instead of 4)
	//speed up everything a bit and update only 3 planes
	for(uint8_t plane = 0; plane < 3; plane++)
	{
		HalVideoSetPlane(plane);
		if(color & planeMask)
			HalVideoState.vmem[offset] = HalVideoState.vmem[offset] | mask;
		else
			HalVideoState.vmem[offset] = HalVideoState.vmem[offset] & ~mask;
		planeMask <<= 1;
	}
#else
	HalVideoState.vmem[HAL_VIDEO_WIDTH * y + x] = color;
#endif
}

#ifdef HAL_VIDEO_USE_640_480
/**
 * @brief Set pixel only in given plane. The plane must be selected first with HalVideoSetPlane()
*/
HOT static inline void HalVideoSetPixelInCurrentPlane(size_t x, size_t y, uint8_t plane, uint8_t color)
{
	if(unlikely((y >= HAL_VIDEO_HEIGHT) || (x >= HAL_VIDEO_WIDTH)))
		return;

	uint32_t offset = HAL_VIDEO_WIDTH * y / 8 + x / 8;
	
	plane &= 3;
	x &= 7;
	uint8_t mask = 0x80 >> x;

	if(color & (1 << plane))
		HalVideoState.vmem[offset] = HalVideoState.vmem[offset] | mask;
	else
		HalVideoState.vmem[offset] = HalVideoState.vmem[offset] & ~mask;
}
#endif


void HalVideoSetPixel(size_t x, size_t y, RtlRGB color)
{
	if(unlikely(!HalVideoState.initialized))
		return;
	HalVideoSetPixelNormalized(x, y, HalVideoNormalizeColor(color));
}

void HalVideoFillScreen(RtlRGB color)
{
	if(unlikely(!HalVideoState.initialized))
		return;

	uint8_t normalizedColor = HalVideoNormalizeColor(color);
#ifdef HAL_VIDEO_USE_640_480
	for(uint8_t p = 0; p < 3; p++)
	{
		HalVideoSetPlane(p);
		for (size_t y = 0; y < HAL_VIDEO_HEIGHT; y++)
		{
			for (size_t x = 0; x < HAL_VIDEO_WIDTH; x++)
			{
				HalVideoSetPixelInCurrentPlane(x, y, p, normalizedColor);
			}
		}
	}
#else
	for (size_t y = 0; y < HAL_VIDEO_HEIGHT; y++)
	{
		for (size_t x = 0; x < HAL_VIDEO_WIDTH; x++)
		{
			HalVideoSetPixelNormalized(x, y, normalizedColor);
		}
	}
#endif
}


void HalVideoClearScreen(void)
{
	if(unlikely(!HalVideoState.initialized))
		return;

#ifdef HAL_VIDEO_USE_640_480
	for(uint8_t p = 0; p < 3; p++)
	{
		HalVideoSetPlane(p);
		for (size_t y = 0; y < HAL_VIDEO_HEIGHT; y++)
		{
			for (size_t x = 0; x < HAL_VIDEO_WIDTH; x++)
			{
				HalVideoSetPixelInCurrentPlane(x, y, p, HalVideoState.bgColor);
			}
		}
	}
#else
	for (size_t y = 0; y < HAL_VIDEO_HEIGHT; y++)
	{
		for (size_t x = 0; x < HAL_VIDEO_WIDTH; x++)
		{
			HalVideoSetPixelNormalized(x, y, HalVideoState.bgColor);
		}
	}
#endif
	HalVideoState.x = 0;
	HalVideoState.y = 0;
}

static inline void HalVideoHandleScroll(size_t objectHeight)
{
	if((HalVideoState.y + objectHeight) <= HAL_VIDEO_HEIGHT)
		return;

#ifdef HAL_VIDEO_USE_640_480
	//in this driver there are only 3 bits used in 16-color mode (instead of 4)
	//speed up everything a bit and update only 3 planes
	for(uint8_t plane = 0; plane < 3; plane++)
	{
		HalVideoSetPlane(plane);
		for(size_t i = objectHeight; i < HAL_VIDEO_HEIGHT; i++)
		{
			RtlMemcpy(&HalVideoState.vmem[HAL_VIDEO_WIDTH * (i - objectHeight) / 8], &HalVideoState.vmem[HAL_VIDEO_WIDTH * i / 8], HAL_VIDEO_WIDTH / 8);
		}
		RtlMemset(&HalVideoState.vmem[HAL_VIDEO_WIDTH * (HAL_VIDEO_HEIGHT - objectHeight) / 8], 0, (objectHeight * HAL_VIDEO_WIDTH) / 8);
	}
#else
	for(size_t i = objectHeight; i < HAL_VIDEO_HEIGHT; i++)
	{
		RtlMemcpy(&HalVideoState.vmem[HAL_VIDEO_WIDTH * (i - objectHeight)], &HalVideoState.vmem[HAL_VIDEO_WIDTH * i], HAL_VIDEO_WIDTH);
	}
	RtlMemset(&HalVideoState.vmem[HAL_VIDEO_WIDTH * (HAL_VIDEO_HEIGHT - objectHeight)], 0, (objectHeight * HAL_VIDEO_WIDTH));

#endif
	HalVideoState.y -= objectHeight;
}

inline void HalVideoPrintChar(char c)
{
	if(unlikely(!HalVideoState.initialized))
		return;

	if('\n' == c)
	{
		HalVideoState.y += (HAL_VIDEO_FONT_HEIGHT + HAL_VIDEO_FONT_LINE_SPACING);
		HalVideoState.x = 0;
		HalVideoHandleScroll(HAL_VIDEO_FONT_HEIGHT + HAL_VIDEO_FONT_LINE_SPACING);
		return;
	}

	if((HalVideoState.x + HAL_VIDEO_FONT_WIDTH) > HAL_VIDEO_WIDTH)
	{
		HalVideoState.y += (HAL_VIDEO_FONT_HEIGHT + HAL_VIDEO_FONT_LINE_SPACING);
		HalVideoState.x = 0;
	}
	HalVideoHandleScroll(HAL_VIDEO_FONT_HEIGHT + HAL_VIDEO_FONT_LINE_SPACING);
	for(uint8_t p = 0; p < 3; p++)
	{
		HalVideoSetPlane(p);
		for(uint8_t h = 0; h < HAL_VIDEO_FONT_HEIGHT; h++)
		{
			for(uint8_t w = 0; w < HAL_VIDEO_FONT_WIDTH; w++)
			{
				HalVideoSetPixelInCurrentPlane((HalVideoState.x + w), (HalVideoState.y + h), p,
					(HalVideoFont8x8[HAL_VIDEO_FONT_HEIGHT * (uint8_t)(c) + h] & ((1 << (HAL_VIDEO_FONT_WIDTH - 1)) >> w)) ? 
					HalVideoState.fgColor : HalVideoState.bgColor);
			}
		}
	}
	HalVideoState.x += HAL_VIDEO_FONT_WIDTH;
}

void HalVideoPrintXY(size_t x, size_t y, const char *s)
{
	if(unlikely(!HalVideoState.initialized))
		return;
		
	HalVideoSetPosition(x, y);

	while('\0' != *s)
		HalVideoPrintChar(*s++);
}

void HalVideoPrint(const char *s)
{
    HalVideoPrintXY(HalVideoState.x, HalVideoState.y, s);
}

void HalVideoSetPosition(size_t x, size_t y)
{
	if(x > HAL_VIDEO_WIDTH)
		x = 0;
	if(y > HAL_VIDEO_HEIGHT)
		y = 0;
	HalVideoState.x = x;
	HalVideoState.y = y;
}

void HalVideoGetCurrentResolution(size_t *x, size_t *y)
{
	*x = HAL_VIDEO_WIDTH;
	*y = HAL_VIDEO_HEIGHT;
}

void HalVideoDisplayBitmap(size_t x, size_t y, const RtlRGB *bitmap, size_t width, size_t height)
{
	if(unlikely(!HalVideoState.initialized))
		return;

	uint32_t index = 0;
	
	for(uint8_t p = 0; p < 3; p++)
	{
		index = 0;
		HalVideoSetPlane(p);
		for(size_t h = 0; h < height; h++)
		{
			for(size_t w = 0; w < width; w++)
			{
				HalVideoSetPixelInCurrentPlane(x + w, y + h, p, HalVideoNormalizeColor(bitmap[index]));
				index++;
			}
		}
	}
}

bool HalVideoIsAvailable(void)
{
	return HalVideoState.initialized;
}

#endif