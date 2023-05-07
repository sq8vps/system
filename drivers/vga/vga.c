#include "vga.h"
#include "../hal/hal.h"
#include "../common.h"

#define PRINTF_INT_MAX_DIGITS 9

uint8_t disp_printChar(uint8_t c, int16_t col, int16_t row, uint8_t type)
{
	if(col >= DISP_MAX_COLS) return 1;
	if(row >= DISP_MAX_ROWS) return 1;

	if(type == 0) type = DISP_COLOR_DEFAULT;

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
		addr = addr / (DISP_MAX_COLS); //liczymy numer rzedu
		addr++; //next row
		addr = DISP_GETADDR(0, addr); //i przechodzimy na poczatek nastepnego
	}
	else
	{
		vmem[addr << 1] = c;
		vmem[(addr << 1) + 1] = type;
		addr++;
	}

	addr = disp_handleScroll(addr);
	disp_setCursor(addr);

	return 0;
}

uint16_t disp_getCursor(void)
{
	Hal_IOPortWriteByte(DISP_PORT_CTRL, 14);
	uint16_t offset = 0;
	offset = Hal_IOPortReadByte(DISP_PORT_DATA) << 8;
	Hal_IOPortWriteByte(DISP_PORT_CTRL, 15);
	offset |= Hal_IOPortReadByte(DISP_PORT_DATA);
	return offset;
}

void disp_setCursor(uint16_t addr)
{
	Hal_IOPortWriteByte(DISP_PORT_CTRL, 14);
	Hal_IOPortWriteByte(DISP_PORT_DATA, (uint8_t)(addr >> 8));
	Hal_IOPortWriteByte(DISP_PORT_CTRL, 15);
	Hal_IOPortWriteByte(DISP_PORT_DATA, (uint8_t)(addr & 0xFF));
}

uint8_t disp_printString(uint8_t *str, int16_t col, int16_t row, uint8_t type)
{
	if(type == 0) type = DISP_COLOR_DEFAULT;

	if(col >= 0 && row >= 0)
	{
		disp_setCursor(DISP_GETADDR(col, row));
	}

	uint16_t i = 0;
	while(str[i] != 0)
	{
		if(disp_printChar(str[i], -1, -1, type)) return 1;
		if(i == 65534) return 1;
		i++;
	}

	return 0;
}

void disp_clear(void)
{
	disp_setCursor(DISP_GETADDR(0, 0));
	for(uint8_t i = 0; i < DISP_MAX_ROWS; i++)
	{
		for(uint8_t j = 0; j < DISP_MAX_COLS; j++)
		{
			disp_printChar(' ', -1, -1, DISP_COLOR_DEFAULT);
		}
	}
	disp_setCursor(DISP_GETADDR(0, 0));
}

uint16_t disp_handleScroll(uint16_t cursor)
{
	if(cursor < (DISP_MAX_ROWS * DISP_MAX_COLS))
	{
		return cursor;
	}
	for(uint8_t i = 1; i < (DISP_MAX_ROWS); i++)
	{
		memcpy((uint8_t*)(DISP_ADDR + (DISP_GETADDR(0, i - 1) << 1)), (uint8_t*)(DISP_ADDR + (DISP_GETADDR(0, i) << 1)), DISP_MAX_COLS << 1);
	}

	uint8_t *last = (uint8_t*)(DISP_ADDR + (DISP_GETADDR(0, DISP_MAX_ROWS - 1) << 1));
	for(uint8_t i = 0; i < DISP_MAX_COLS * 2; i++)
	{
		last[i] = 0;
	}

	cursor -= DISP_MAX_COLS;

	return cursor;
}

uint8_t disp_printHex(uint64_t num, int16_t col, int16_t row, uint8_t type)
{
	if(type == 0) type = DISP_COLOR_DEFAULT;

	if(col >= 0 && row >= 0)
	{
		disp_setCursor(DISP_GETADDR(col, row));
	}

	disp_printChar('0', -1, -1, type);
	disp_printChar('x', -1, -1, type);
	for(uint8_t i = 0; i < 16; i++)
	{
		uint8_t t = (num >> ((15 - i) << 2) & 0xF);
		if(t < 10)
			disp_printChar(t + 48, -1, -1, type);
		else
			disp_printChar(t + 55, -1, -1, type);

	}
	return 0;
}

uint8_t disp_printHex16(uint16_t num, int16_t col, int16_t row, uint8_t type)
{
	if(type == 0) type = DISP_COLOR_DEFAULT;

	if(col >= 0 && row >= 0)
	{
		disp_setCursor(DISP_GETADDR(col, row));
	}

	disp_printChar('0', -1, -1, type);
	disp_printChar('x', -1, -1, type);
	for(uint8_t i = 0; i < 4; i++)
	{
		uint8_t t = (num >> ((3 - i) << 2) & 0xF);
		if(t < 10)
			disp_printChar(t + 48, -1, -1, type);
		else
			disp_printChar(t + 55, -1, -1, type);

	}
	return 0;
}

uint8_t disp_printHex8(uint8_t num, int16_t col, int16_t row, uint8_t type)
{
	if(type == 0) type = DISP_COLOR_DEFAULT;

	if(col >= 0 && row >= 0)
	{
		disp_setCursor(DISP_GETADDR(col, row));
	}

	disp_printChar('0', -1, -1, type);
	disp_printChar('x', -1, -1, type);
	if(((num & 0xF0) >> 4) > 9)
		disp_printChar(((num & 0xF0) >> 4) + 55, -1, -1, type);
	else
		disp_printChar(((num & 0xF0) >> 4) + 48, -1, -1, type);

	if(((num & 0xF)) > 9)
		disp_printChar(((num & 0xF)) + 55, -1, -1, type);
	else
		disp_printChar(((num & 0xF)) + 48, -1, -1, type);

	return 0;
}


/**
 * \brief Prints formatted input (simplified printf)
 * \param *format Input format
 * \param ... Formatting parameters
 * \warning This function does NOT support any parameters other than the specifier. Additionally, it supports only d, i, u, o, x, X, c, s and % specifiers.
 */
int printf(const char *format, ...)
{

	va_list args;
	va_start(args, format); //argument list starts after format argument

	int ret = 0;

	while(*format != '\0')
	{
		if(*format == '%')
		{
			if((*(format + 1) == 'd') || (*(format + 1) == 'i') || (*(format + 1) == 'u')) //signed/unsigned decimal integer
			{
				int a = va_arg(args, int);
				int t = (a > 0) ? a : -a; //abs(x)
				if((a < 0) && (*(format + 1) != 'u'))
					disp_printChar('-', DISP_NEXT, DISP_NEXT, DISP_COLOR_DEFAULT);
				uint8_t i = PRINTF_INT_MAX_DIGITS;
				for(; i > 1; i--) //skip zeros
				{
					uint32_t p = (uint32_t)pow10(i - 1);
					if(t / p != 0)
						break;
					t %= p;
				}
				for(; i > 0; i--)
				{
					uint32_t p = (uint32_t)pow10(i - 1);
					disp_printChar((t / p) + 48, DISP_NEXT, DISP_NEXT, DISP_COLOR_DEFAULT);
					t %= p;
				}
			}
			else if(*(format + 1) == 'o') //octal
			{
				uint64_t a = (uint64_t)va_arg(args, int);
				uint8_t wasNonZero = 0;
				for(uint8_t i = 0; i < 22; i++) //64 bits maximum
				{
					uint8_t t = ((a >> ((21 - i) * 3)) & 7);
					if(t == 0 && wasNonZero == 0 && i < 21) continue;
					wasNonZero = 1;
					disp_printChar(t + 48, DISP_NEXT, DISP_NEXT, DISP_COLOR_DEFAULT);
				}
			}
			else if((*(format + 1) == 'x') || (*(format + 1) == 'X')) //lowercase/uppercase hex
			{
				uint64_t a = (uint64_t)va_arg(args, unsigned int);
				uint8_t wasNonZero = 0;
				for(uint8_t i = 0; i < 16; i++) //16 hex characters max (64 bits)
				{
					uint8_t t = ((a >> ((15 - i) << 2)) & 0xF);
					if(t == 0 && wasNonZero == 0 && i < 15) continue;
					wasNonZero = 1;
					if(t < 10)
						disp_printChar(t + 48, DISP_NEXT, DISP_NEXT, DISP_COLOR_DEFAULT);
					else
						disp_printChar(t + ((*(format + 1) == 'x') ? 87 : 55), DISP_NEXT, DISP_NEXT, DISP_COLOR_DEFAULT);
				}
			}
			else if(*(format + 1) == 'c') //character
			{
				disp_printChar((char)va_arg(args, int), DISP_NEXT, DISP_NEXT, DISP_COLOR_DEFAULT);
			}
			else if(*(format + 1) == 's') //string
			{
				disp_printString(va_arg(args, char*), DISP_NEXT, DISP_NEXT, DISP_COLOR_DEFAULT);
			}
			else if(*(format + 1) == '%') //percent
			{
				disp_printChar('%', DISP_NEXT, DISP_NEXT, DISP_COLOR_DEFAULT);
			}
			format++;
		}
		else
			disp_printChar(*format, DISP_NEXT, DISP_NEXT, DISP_COLOR_DEFAULT);

		format++;
	}
	va_end(args);
	return 0;
}
