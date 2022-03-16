#include "disp.h"

#define DISP_GETADDR(c, r) (((((r) * DISP_MAX_ROWS) + (c)) * 2))

uint8_t disp_printChar(uint8_t c, int16_t col, int16_t row, uint8_t type)
{
	if(col >= DISP_MAX_COLS) return 1;
	if(row >= DISP_MAX_ROWS) return 1;

	if(type == 0) type = DISP_COLOR_WHITE_ON_BLACK;

	uint8_t *vmem = (uint8_t*)DISP_ADDR;
	uint16_t addr = 0;
	if(col >= 0 && row >= 0)
	{
		addr = DISP_GETADDR(col, row);
	}
	else
	{
		addr = disp_getCursor();
	}

	if(c == '\n')
	{
		addr = addr / (DISP_MAX_COLS * 2); //liczymy numer rzedu
		addr = DISP_GETADDR(DISP_MAX_COLS - 1, row); //i przechodzimy na koniec tego rzedu
	}
	else
	{
		vmem[addr + 2] = c;
		vmem[addr + 3] = type;
	}

	addr += 2;
	addr = disp_handleScroll(addr);
	disp_setCursor(addr);

	return 0;
}

uint16_t disp_getCursor(void)
{
	port_writeByte(DISP_PORT_CTRL, 14);
	uint16_t offset = 0;
	offset = port_readByte(DISP_PORT_DATA) << 8;
	port_writeByte(DISP_PORT_CTRL, 15);
	offset |= port_readByte(DISP_PORT_DATA);
	return offset * 2;
}

void disp_setCursor(uint16_t addr)
{
	addr /= 2;
	port_writeByte(DISP_PORT_CTRL, 14);
	port_writeByte(DISP_PORT_DATA, (uint8_t)(addr >> 8));
	port_writeByte(DISP_PORT_CTRL, 15);
	port_writeByte(DISP_PORT_DATA, (uint8_t)(addr & 0xFF));
}

uint8_t sp[14] = "0123456789abc";

uint8_t disp_printString(uint8_t *str, int16_t col, int16_t row)
{
	if(col >= 0 && row >= 0)
	{
		//disp_setCursor(DISP_GETADDR(col, row));
	}

	uint8_t *s = &sp[0];
	//s += 0x13c8;
	uint16_t i = 0;
	while(i < 10)
	{
		//if(disp_printChar(str[i], col, row, DISP_COLOR_WHITE_ON_BLACK)) return 1;
		if(i == 65534) return 1;
		disp_printChar(s[i], -1, 0, 0);
		i++;
	}

	disp_printChar((((uint32_t)s & 0xF0000000) >> 28) + 48, -1, 0, 0);
	disp_printChar((((uint32_t)s & 0xF000000) >> 24) + 48, -1, 0, 0);
	disp_printChar((((uint32_t)s & 0xF00000) >> 20) + 48, -1, 0, 0);
	disp_printChar((((uint32_t)s & 0xF0000) >> 16) + 48, -1, 0, 0);
	disp_printChar((((uint32_t)s & 0xF000) >> 12) + 48, -1, 0, 0);
	disp_printChar((((uint32_t)s & 0xF00) >> 8) + 48, -1, 0, 0);
	disp_printChar((((uint32_t)s & 0xF0) >> 4) + 48, -1, 0, 0);
	disp_printChar((((uint32_t)s & 0xF)) + 48, -1, 0, 0);
	disp_printChar('T', -1, 0, 0);

	return 0;
}

void disp_clear(void)
{
	for(uint8_t i = 0; i < DISP_MAX_ROWS; i++)
	{
		for(uint8_t j = 0; j < DISP_MAX_COLS; j++)
		{
			disp_printChar(' ', j, i, DISP_COLOR_WHITE_ON_BLACK);
		}
	}

	disp_setCursor(0);
}

uint16_t disp_handleScroll(uint16_t cursor)
{
	if(cursor < (DISP_MAX_ROWS * DISP_MAX_COLS * 2))
	{
		return cursor;
	}
	for(uint8_t i = 0; i < DISP_MAX_ROWS; i++)
	{
		memcpy((uint8_t*)(DISP_ADDR + DISP_GETADDR(0, i)), (uint8_t*)(DISP_ADDR + DISP_GETADDR(0, i - 1)), DISP_MAX_COLS * 2);
	}

	uint8_t *last = (uint8_t*)(DISP_ADDR + DISP_GETADDR(0, DISP_MAX_ROWS - 1));
	for(uint8_t i = 0; i < DISP_MAX_COLS * 2; i++)
	{
		last[i] = 0;
	}

	cursor -= 2 * DISP_MAX_COLS;

	return cursor;
}
