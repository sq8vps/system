#include "tmvga.h"
#include "hal/hal.h" //kernel HAL library

#define DISP_ADDR 0xB8000 //text mode VGA buffer address
#define DISP_PORT_CTRL 0x3D4 //control port number
#define DISP_PORT_DATA 0x3D5 //data port number

#define DISP_MAX_ROWS 25 //number of rows
#define DISP_MAX_COLS 80 //number of columns

#define CMD_CURSOR_LOCATION_HIGH 0xE
#define CMD_CURSOR_LOCATION_LOW 0xF

#define DEFAULT_FG_COLOR 0xF //white
#define DEFAULT_BG_COLOR 0x0 //black

static inline uint8_t* getAddr(uint32_t col, uint32_t row)
{
    return (uint8_t*)(DISP_ADDR + 2 * ((row * DISP_MAX_COLS) + col));
}

static void handleScroll(uint32_t *col, uint32_t *row)
{
	if((*col < DISP_MAX_COLS) && (*row < DISP_MAX_ROWS))
		return;

	for(uint32_t i = 1; i < (DISP_MAX_ROWS); i++)
	{
		Cm_memcpy(getAddr(0, i - 1), getAddr(0, i), DISP_MAX_COLS * 2);
	}

	uint8_t *last = getAddr(0, DISP_MAX_ROWS - 1);
	for(uint32_t i = 0; i < DISP_MAX_COLS; i++)
	{
		last[2 * i] = 0;
        last[2 * i + 1] = 0;
	}

	(*row)--;
}

struct VGA16color
{
	uint8_t r, g, b;
	uint8_t value;
};

struct VGA16color VGA16colorTable[] = 
{
	{.r = 0, .g = 0, .b = 0, .value = 0}, //black
	{.r = 0, .g = 0, .b = 255, .value = 1}, //blue
	{.r = 0, .g = 255, .b = 0, .value = 2}, //green
	{.r = 0, .g = 255, .b = 255, .value = 3}, //cyan
	{.r = 255, .g = 0, .b = 0, .value = 4}, //red
	{.r = 255, .g = 0, .b = 255, .value = 5}, //magenta
	{.r = 150, .g = 75, .b = 0, .value = 6}, //brown
	{.r = 211, .g = 211, .b = 211, .value = 7}, //light gray
	{.r = 169, .g = 169, .b = 169, .value = 8}, //dark gray
	{.r = 173, .g = 216, .b = 230, .value = 9}, //light blue
	{.r = 144, .g = 238, .b = 144, .value = 10}, //light green
	{.r = 224, .g = 255, .b = 255, .value = 11}, //light cyan
	{.r = 255, .g = 204, .b = 203, .value = 12}, //light red
	{.r = 255, .g = 128, .b = 255, .value = 13}, //light magenta
	{.r = 255, .g = 255, .b = 0, .value = 14}, //yellow
	{.r = 255, .g = 255, .b = 255, .value = 15}, //white
};

uint32_t decodeColor(Cm_RGBA_t in)
{
	//convert to integer RGB values
	uint8_t r = in.r * 255.f;
	uint8_t g = in.g * 255.f;
	uint8_t b = in.b * 255.f;

	uint8_t closestColor = 0;
	uint32_t smallestDistance = 0xFFFFFFFF;
	for(uint8_t i = 0; i < (sizeof(VGA16colorTable) / sizeof(*VGA16colorTable)); i++)
	{
		uint32_t distance = 0;
		distance = Cm_abs((int16_t)VGA16colorTable[i].r - (int16_t)r)
				+ Cm_abs((int16_t)VGA16colorTable[i].g - (int16_t)g)
				+ Cm_abs((int16_t)VGA16colorTable[i].b - (int16_t)b);
		if(distance < smallestDistance)
		{
			smallestDistance = distance;
			closestColor = i;
		}
	}
	return closestColor;
}

void Tmvga_putChar(char c, Cm_RGBA_t fg, Cm_RGBA_t bg)
{
	uint32_t col, row;
    Tmvga_getCursor(&col, &row);

	if(c == '\n') //newline
	{
		row++; //next row
        row %= DISP_MAX_ROWS;
	}
	else
	{
		getAddr(col, row)[0] = c;
		getAddr(col, row)[1] = (decodeColor(fg) & 0xF) | ((decodeColor(bg) & 0x7) << 4);
        col++;
        if(col >= DISP_MAX_COLS)
		{
			row++;
			col = 0;
		}
	}

	handleScroll(&col, &row);
	Tmvga_setCursor(col, row);
}

void Tmvga_getCursor(uint32_t *col, uint32_t *row)
{
	Hal_IOPortWriteByte(DISP_PORT_CTRL, CMD_CURSOR_LOCATION_HIGH);
	uint16_t offset = 0;
	offset = Hal_IOPortReadByte(DISP_PORT_DATA) << 8;
	Hal_IOPortWriteByte(DISP_PORT_CTRL, CMD_CURSOR_LOCATION_LOW);
	offset |= Hal_IOPortReadByte(DISP_PORT_DATA);
	
    *row = offset / DISP_MAX_COLS;
    *col = offset % DISP_MAX_COLS;
}

void Tmvga_setCursor(uint32_t col, uint32_t row)
{
	uint16_t offset = row * DISP_MAX_COLS + col;
    Hal_IOPortWriteByte(DISP_PORT_CTRL, CMD_CURSOR_LOCATION_HIGH);
	Hal_IOPortWriteByte(DISP_PORT_DATA, (uint8_t)(offset >> 8));
	Hal_IOPortWriteByte(DISP_PORT_CTRL, CMD_CURSOR_LOCATION_LOW);
	Hal_IOPortWriteByte(DISP_PORT_DATA, (uint8_t)(offset & 0xFF));
}

void Tmvga_resetCursor(void)
{
    Tmvga_setCursor(0, 0);
}

void Tmvga_clear(void)
{
	Tmvga_resetCursor();
	for(uint8_t i = 0; i < DISP_MAX_ROWS; i++)
	{
		for(uint8_t j = 0; j < DISP_MAX_COLS; j++)
		{
			Cm_RGBA_t black = {.r = 0.f, .g = 0.f, .b = 0.f};
			Tmvga_putChar(' ', black, black);
		}
	}
	Tmvga_resetCursor();
}

