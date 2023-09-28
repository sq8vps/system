

#include "bootvga.h"
#include "hal/ioport.h"
#include "mm/mmio.h"
#include "font.h"
#include "common.h"

//uncomment to enable 640x480 mode
//otherwise 320x200 mode is used
//640x480 is slower, because it uses IO ports and all these VGA planes
#define BOOT_VGA_USE_640_480

#ifdef BOOT_VGA_USE_640_480
	#define BOOTVGA_FRAME_BUFFER_ADDRESS 0xA0000
	#define BOOTVGA_WIDTH 640
	#define BOOTVGA_HEIGHT 480
#else
	#define BOOTVGA_FRAME_BUFFER_ADDRESS 0xA0000
	#define BOOTVGA_WIDTH 320
	#define BOOTVGA_HEIGHT 200
#endif

#define BOOTVGA_AC_INDEX 0x3C0
#define BOOTVGA_AC_WRITE 0x3C0
#define BOOTVGA_AC_READ 0x3C1
#define BOOTVGA_MISC_WRITE 0x3C2
#define BOOTVGA_SEQ_INDEX 0x3C4
#define BOOTVGA_SEQ_DATA 0x3C5
#define BOOTVGA_DAC_READ_INDEX 0x3C7
#define BOOTVGA_DAC_WRITE_INDEX 0x3C8
#define BOOTVGA_DAC_DATA 0x3C9
#define BOOTVGA_MISC_READ 0x3CC
#define BOOTVGA_GC_INDEX 0x3CE
#define BOOTVGA_GC_DATA 0x3CF
#define BOOTVGA_CRTC_INDEX 0x3D4
#define BOOTVGA_CRTC_DATA 0x3D5
#define BOOTVGA_INSTAT_READ 0x3DA
#define BOOTVGA_NUM_SEQ_REGS 5
#define BOOTVGA_NUM_CRTC_REGS 25
#define BOOTVGA_NUM_GC_REGS 9
#define BOOTVGA_NUM_AC_REGS 21
#define BOOTVGA_NUM_REGS (1 + BOOTVGA_NUM_SEQ_REGS + BOOTVGA_NUM_CRTC_REGS + BOOTVGA_NUM_GC_REGS + BOOTVGA_NUM_AC_REGS)

#ifdef BOOT_VGA_USE_640_480
static uint8_t currentBgColor = 0b000; //black
static uint8_t currentFgColor = 0b111; //white
#else
static uint8_t currentBgColor = 0; //black
static uint8_t currentFgColor = 0b111111; //white
#endif

static uint16_t currentX = 0; //current X position when writing text
static uint16_t currentY = 0; //current Y position when wiriting

static uint8_t *vmem = NULL; //mapped video memory pointerS

#ifdef BOOT_VGA_USE_640_480
static uint8_t bootVga640x480x16[] =
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
static uint8_t bootVga320x200x256[] =
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




static void setupVgaRegisters(uint8_t *regs)
{
	uint16_t i = 0;

    /* write MISCELLANEOUS reg */
	PortIoWriteByte(BOOTVGA_MISC_WRITE, *regs);
	regs++;

    /* unlock CRTC registers */
	for (i = 0; i < BOOTVGA_NUM_SEQ_REGS; i++)
	{
		PortIoWriteByte(BOOTVGA_SEQ_INDEX, i);
		PortIoWriteByte(BOOTVGA_SEQ_DATA, *regs);
		regs++;
	}
	/* unlock CRTC registers */
	PortIoWriteByte(BOOTVGA_CRTC_INDEX, 0x03);
	PortIoWriteByte(BOOTVGA_CRTC_DATA, PortIoReadByte(BOOTVGA_CRTC_DATA) | 0x80);
	PortIoWriteByte(BOOTVGA_CRTC_INDEX, 0x11);
	PortIoWriteByte(BOOTVGA_CRTC_DATA, PortIoReadByte(BOOTVGA_CRTC_DATA) & ~0x80);
	/* make sure they remain unlocked */
	regs[0x03] |= 0x80;
	regs[0x11] &= ~0x80;
	/* write CRTC regs */
	for (i = 0; i < BOOTVGA_NUM_CRTC_REGS; i++)
	{
		PortIoWriteByte(BOOTVGA_CRTC_INDEX, i);
		PortIoWriteByte(BOOTVGA_CRTC_DATA, *regs);
		regs++;
	}
	/* write GRAPHICS CONTROLLER regs */
	for (i = 0; i < BOOTVGA_NUM_GC_REGS; i++)
	{
		PortIoWriteByte(BOOTVGA_GC_INDEX, i);
		PortIoWriteByte(BOOTVGA_GC_DATA, *regs);
		regs++;
	}
	/* write ATTRIBUTE CONTROLLER regs */
	for (i = 0; i < BOOTVGA_NUM_AC_REGS; i++)
	{
		PortIoReadByte(BOOTVGA_INSTAT_READ);
		PortIoWriteByte(BOOTVGA_AC_INDEX, i);
		PortIoWriteByte(BOOTVGA_AC_WRITE, *regs);
		regs++;
	}

	/* lock 16-color palette and unblank display */
	PortIoReadByte(BOOTVGA_INSTAT_READ);
	PortIoWriteByte(BOOTVGA_AC_INDEX, 0x20);
}

static void writeColorPalette(void)
{
	PortIoWriteByte(BOOTVGA_AC_INDEX, 0x00); //enable access to color palette
    PortIoWriteByte(BOOTVGA_DAC_WRITE_INDEX, 0);
    
    uint8_t r = 0, g = 0, b = 0;
#ifdef BOOT_VGA_USE_640_480
    for(uint16_t i = 0; i < 16; i++)
    {
        PortIoWriteByte(BOOTVGA_DAC_DATA, r);
        PortIoWriteByte(BOOTVGA_DAC_DATA, g);
        PortIoWriteByte(BOOTVGA_DAC_DATA, b);

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
    for(uint16_t i = 0; i < 256; i++)
    {
        PortIoWriteByte(BOOTVGA_DAC_DATA, r);
        PortIoWriteByte(BOOTVGA_DAC_DATA, g);
        PortIoWriteByte(BOOTVGA_DAC_DATA, b);
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
	PortIoWriteByte(BOOTVGA_AC_INDEX, 0x20); //disable access to color palette
}


STATUS BootVgaInit(void)
{	
	if(NULL == vmem)
	{
		vmem = MmMapMmIo(BOOTVGA_FRAME_BUFFER_ADDRESS, BOOTVGA_WIDTH * BOOTVGA_HEIGHT);
		if(NULL == vmem)
			return BOOTVGA_INIT_FAILURE;
	}

#ifdef BOOT_VGA_USE_640_480
	setupVgaRegisters(bootVga640x480x16);
#else
	setupVgaRegisters(bootVga320x200x256);
#endif
	writeColorPalette();

	BootVgaClearScreen();

	return OK;
}


void BootVgaDeinit(void)
{
    if(NULL != vmem)
	{
        MmUnmapMmIo(vmem, BOOTVGA_WIDTH * BOOTVGA_HEIGHT);
	}

	PortIoWriteByte(BOOTVGA_GC_INDEX, 0x08);
    PortIoWriteByte(BOOTVGA_GC_DATA, 0xff);
}

#ifdef BOOT_VGA_USE_640_480
static inline void setPlane(uint8_t plane)
{
	plane &= 3;
	uint8_t mask = 1 << plane;

	PortIoWriteByte(BOOTVGA_GC_INDEX, 4);
	PortIoWriteByte(BOOTVGA_GC_DATA, plane);

	PortIoWriteByte(BOOTVGA_SEQ_INDEX, 2);
	PortIoWriteByte(BOOTVGA_SEQ_DATA, mask);
}
#endif

static inline uint8_t normalizeColor(CmRGB color)
{
#ifdef BOOT_VGA_USE_640_480
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

void BootVgaSetBackgroundColor(CmRGB color)
{
	currentBgColor = normalizeColor(color);
}

void BootVgaSetForegroundColor(CmRGB color)
{
	currentFgColor = normalizeColor(color);
}

void BootVgaSetColor(CmRGB fg, CmRGB bg)
{
	BootVgaSetForegroundColor(fg);
	BootVgaSetBackgroundColor(bg);
}

static inline void setPixel(uint16_t x, uint16_t y, uint8_t color)
{	
	if((y >= BOOTVGA_HEIGHT) || (x >= BOOTVGA_WIDTH))
		return;

#ifdef BOOT_VGA_USE_640_480
	uint16_t planeMask = 1;
	uint32_t offset = BOOTVGA_WIDTH * y / 8 + x / 8;
	
	x &= 7;
	uint8_t mask = 0x80 >> x;
	//in this driver there are only 3 bits used in 16-color mode (instead of 4)
	//speed up everything a bit and update only 3 planes
	for(uint8_t plane = 0; plane < 3; plane++)
	{
		setPlane(plane);
		if(color & planeMask)
			vmem[offset] = vmem[offset] | mask;
		else
			vmem[offset] = vmem[offset] & ~mask;
		planeMask <<= 1;
	}
#else
	vmem[BOOTVGA_WIDTH * y + x] = color;
#endif
}

#ifdef BOOT_VGA_USE_640_480
/**
 * @brief Set pixel only in given plane. The plane must be selected first with setPlane()
*/
static inline void setPixelInCurrentPlane(uint16_t x, uint16_t y, uint8_t plane, uint8_t color)
{
	if((y >= BOOTVGA_HEIGHT) || (x >= BOOTVGA_WIDTH))
		return;

	uint32_t offset = BOOTVGA_WIDTH * y / 8 + x / 8;
	
	plane &= 3;
	x &= 7;
	uint8_t mask = 0x80 >> x;

	if(color & (1 << plane))
		vmem[offset] = vmem[offset] | mask;
	else
		vmem[offset] = vmem[offset] & ~mask;

}
#endif

void BootVgaSetPixel(uint16_t x, uint16_t y, CmRGB color)
{
	setPixel(x, y, normalizeColor(color));
}

void BootVgaFillScreen(CmRGB color)
{
	if(NULL == vmem)
		return;

	uint8_t normalizedColor = normalizeColor(color);
#ifdef BOOT_VGA_USE_640_480
	for(uint8_t p = 0; p < 3; p++)
	{
		setPlane(p);
		for (uint16_t y = 0; y < BOOTVGA_HEIGHT; y++)
		{
			for (uint16_t x = 0; x < BOOTVGA_WIDTH; x++)
			{
				setPixelInCurrentPlane(x, y, p, normalizedColor);
			}
		}
	}
#else
	for (uint16_t y = 0; y < BOOTVGA_HEIGHT; y++)
	{
		for (uint16_t x = 0; x < BOOTVGA_WIDTH; x++)
		{
			setPixel(x, y, normalizedColor);
		}
	}
#endif
}


void BootVgaClearScreen(void)
{
	if(NULL == vmem)
		return;

#ifdef BOOT_VGA_USE_640_480
	for(uint8_t p = 0; p < 3; p++)
	{
		setPlane(p);
		for (uint16_t y = 0; y < BOOTVGA_HEIGHT; y++)
		{
			for (uint16_t x = 0; x < BOOTVGA_WIDTH; x++)
			{
				setPixelInCurrentPlane(x, y, p, currentBgColor);
			}
		}
	}
#else
	for (uint16_t y = 0; y < BOOTVGA_HEIGHT; y++)
	{
		for (uint16_t x = 0; x < BOOTVGA_WIDTH; x++)
		{
			setPixel(x, y, currentBgColor);
		}
	}
#endif
	currentX = 0;
	currentY = 0;
}

static void handleScroll(uint16_t objectHeight)
{
	if(NULL == vmem)
		return;

	if((currentY + objectHeight) <= BOOTVGA_HEIGHT)
		return;

#ifdef BOOT_VGA_USE_640_480
	//in this driver there are only 3 bits used in 16-color mode (instead of 4)
	//speed up everything a bit and update only 3 planes
	for(uint8_t plane = 0; plane < 3; plane++)
	{
		setPlane(plane);
		for(uint16_t i = objectHeight; i < BOOTVGA_HEIGHT; i++)
		{
			CmMemcpy(&vmem[BOOTVGA_WIDTH * (i - objectHeight) / 8], &vmem[BOOTVGA_WIDTH * i / 8], BOOTVGA_WIDTH / 8);
		}
		CmMemset(&vmem[BOOTVGA_WIDTH * (BOOTVGA_HEIGHT - objectHeight) / 8], 0, (objectHeight * BOOTVGA_WIDTH) / 8);
	}
#else
	for(uint16_t i = objectHeight; i < BOOTVGA_HEIGHT; i++)
	{
		CmMemcpy(&vmem[BOOTVGA_WIDTH * (i - objectHeight)], &vmem[BOOTVGA_WIDTH * i], BOOTVGA_WIDTH);
	}
	CmMemset(&vmem[BOOTVGA_WIDTH * (BOOTVGA_HEIGHT - objectHeight)], 0, (objectHeight * BOOTVGA_WIDTH));

#endif
	currentY -= objectHeight;
}

void BootVgaPrintStringXY(uint16_t x, uint16_t y, char *s)
{
	if(NULL == vmem)
		return;

	currentX = x;
	currentY = y;
	while(*s)
	{
		BootVgaPrintChar(*s);
		s++;
	}
}

void BootVgaPrintString(char *s)
{
	BootVgaPrintStringXY(currentX, currentY, s);
}

void BootVgaPrintChar(char c)
{
	if(NULL == vmem)
		return;

	if('\n' == c)
	{
		currentY += (BOOTVGA_FONT_HEIGHT + BOOTVGA_FONT_LINE_SPACING);
		currentX = 0;
		handleScroll(BOOTVGA_FONT_HEIGHT + BOOTVGA_FONT_LINE_SPACING);
		return;
	}

	if((currentX + BOOTVGA_FONT_WIDTH) > BOOTVGA_WIDTH)
	{
		currentY += (BOOTVGA_FONT_HEIGHT + BOOTVGA_FONT_LINE_SPACING);
		currentX = 0;
	}
	handleScroll(BOOTVGA_FONT_HEIGHT + BOOTVGA_FONT_LINE_SPACING);
	for(uint8_t p = 0; p < 3; p++)
	{
		setPlane(p);
		for(uint8_t h = 0; h < BOOTVGA_FONT_HEIGHT; h++)
		{
			for(uint8_t w = 0; w < BOOTVGA_FONT_WIDTH; w++)
			{
				setPixelInCurrentPlane((currentX + w), (currentY + h), p,
					(BootVgaFont8x8[BOOTVGA_FONT_HEIGHT * (uint8_t)(c) + h] & ((1 << (BOOTVGA_FONT_WIDTH - 1)) >> w)) ? 
					currentFgColor : currentBgColor);
			}
		}
	}
	currentX += BOOTVGA_FONT_WIDTH;
}

void BootVgaSetPosition(uint16_t x, uint16_t y)
{
	if(x > BOOTVGA_WIDTH)
		x = 0;
	if(y > BOOTVGA_HEIGHT)
		y = 0;
	currentX = x;
	currentY = y;
}

void BootVgaGetCurrentResolution(uint16_t *x, uint16_t *y)
{
	*x = BOOTVGA_WIDTH;
	*y = BOOTVGA_HEIGHT;
}

void BootVgaDisplayBitmap(uint16_t x, uint16_t y, CmRGB *bitmap, uint16_t width, uint16_t height)
{
	if(NULL == vmem)
		return;

	uint32_t index = 0;
	
	for(uint8_t p = 0; p < 3; p++)
	{
		index = 0;
		setPlane(p);
		for(uint16_t h = 0; h < height; h++)
		{
			for(uint16_t w = 0; w < width; w++)
			{
				setPixelInCurrentPlane(x + w, y + h, p, normalizeColor(bitmap[index]));
				index++;
			}
		}
	}
}