#include "common.h"
#include "sdrv/bootvga/bootvga.h"
#include <stdbool.h>
#include <stdarg.h>

static unsigned int findHighestPosition(unsigned int n)
{
    unsigned int i = 1;
    while((i * 10) < n)
        i *= 10;
    
    return i;
}

int CmPrintf(const char *format, ...)
{
	va_list args;
	va_start(args, format); //argument list starts after the format argument

	int written = 0;

	while(*format)
	{
		if('%' == *format)
		{
            switch(format[1])
            {
                //signed decimal integer
                case 'd':
                case 'i':
                    int valueS = va_arg(args, int);
                    unsigned int absolute = CmAbs(valueS);
                    if(valueS < 0)
                    {
                        BootVgaPrintChar('-');
                        written++;
                    }
                //unsigned decimal integer
                case 'u':
                    unsigned int position = findHighestPosition(absolute);
                    while(position)
                    {
                        unsigned int number = absolute / position;
                        BootVgaPrintChar((char)(number + 48));
                        absolute -= (number * position);
                        position /= 10;
                        written++;
                    }
                    break;
                //octal
                case 'o':
                    unsigned int valueU = va_arg(args, unsigned int);
                    bool wasNonZero = false;
                    //neither 16 nor 32 nor 64 divides by 3, so 1 must be added
                    for(uint8_t i = 8 * sizeof(unsigned int) / 3 + 1; i > 0; i--) 
                    {
                        uint8_t oct = (valueU >> (3 * (i - 1))) & 0x7;
                        if((0 == oct) && !wasNonZero && (i > 1))
                            continue;
                        wasNonZero = true;
                        BootVgaPrintChar((char)(oct + 48));
                        written++;
                    }
                    break;
                //hexadecimal
                case 'X':
                case 'x':
                    valueU = va_arg(args, unsigned int);
                    wasNonZero = false;
                    for(uint8_t i = 8 * sizeof(unsigned int) / 4; i > 0; i--)
                    {
                        uint8_t hex = (valueU >> (4 * (i - 1))) & 0xF;
                        if((0 == hex) && !wasNonZero && (i > 1))
                            continue;
                        wasNonZero = true;
                        if(hex < 10)
                            BootVgaPrintChar((char)(hex + 48));
                        else
                            BootVgaPrintChar((char)(hex + (('X' == format[1]) ? 55 : 87)));
                        written++;
                    }
                    break;
                //character
                case 'c':
                    BootVgaPrintChar((char)va_arg(args, int));
                    written++;
                    break;
                //string
                case 's':
                    BootVgaPrintString(va_arg(args, char*));
                    written++;
                    break;
                //percent sign
                case '%':
                    BootVgaPrintChar('%');
                    written++;
                    break;
                default:
                    BootVgaPrintChar('?');
                    written++;
                    break;
            }
			format++;
		}
		else
        {
			BootVgaPrintChar(*format);
            written++;
        }

		format++;
	}
	va_end(args);
	return written;
}