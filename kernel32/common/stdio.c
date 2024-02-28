#include "stdio.h"
#include "common.h"
#include "sdrv/bootvga/bootvga.h"
#include <stdbool.h>

#define dabs(x) ((x) > 0.0 ? (x) : (-x))

static uint8_t findHighestPosition(uint64_t n, uint64_t *power)
{
    if(0 == n)
    {
        *power = 1;
        return 1;
    }
    uint64_t i = 0;
    uint64_t k = 1;
    while(n >= 10)
    {
        n /= 10;
        k *= 10;
        i++;
    }
    *power = k;
    return i;
}

static uint8_t findHighestPositionBase2(uint64_t n, uint8_t baseExponent)
{
    uint64_t i = 0;
    uint64_t k = 1;
    while((k << baseExponent) < n)
    {
        k <<= baseExponent;
        i++;
    }
    return i;
}

static double doubleToScientific(double x, int16_t *exponent)
{
    bool sign = x > 0.0L;
    x = dabs(x);
    *exponent = 0;
    if(x >= 10.0L)
    {
        while(x >= 10.0L)
        {
            (*exponent)++;
            x *= 0.1;
        }
    }
    else
    {
        while(x < 1.0L)
        {
            (*exponent)--;
            x *= 10.0L;
        }
    }
    return sign ? x : -x;
}

static int printfPad(uint32_t count, bool zeroPad)
{
    int written = 0;
    for(uint32_t i = 0; i < count; i++)
    {
        if(zeroPad)
            BootVgaPrintChar('0'); 
        else
            BootVgaPrintChar(' '); 
        written++;
    }
    return written;
}

enum printfFlag
{
    NO_FLAG,
    LEFT_JUSTIFY,
    SIGN,
    SIGN_REPLACE,
    DECIMAL,
    ZERO_PAD,
};

static int printfBase10(uint64_t x, bool sign, uint32_t width, uint32_t precision, enum printfFlag flag)
{
    int written = 0;
    uint64_t power = 0;
    uint32_t length = findHighestPosition(x, &power);
    uint32_t precisionPad = 0;
    
    if(precision > length)
    {
        precisionPad = precision - length;
        length = precision; 
    }

    if(!sign || (SIGN == flag) || (SIGN_REPLACE == flag))
        length++;

    if(!sign)
    {
        BootVgaPrintChar('-');
        written++;
    }
    else
    {
        if(SIGN == flag)
        {
            BootVgaPrintChar('+');
            written++;
        }
        else if(SIGN_REPLACE == flag)
        {
            BootVgaPrintChar(' ');
            written++;
        }
    }
    
    if((flag != LEFT_JUSTIFY) && (length < width))
    {
        written += printfPad(width - length, ZERO_PAD == flag);
    }

    if(precisionPad)
    {
        written += printfPad(precisionPad, true); //always pad with zeros
    }
    
    while(power)
    {
        uint8_t number = x / power;
        BootVgaPrintChar((char)(number + '0'));
        x -= (number * power);
        power /= 10;
        written++;
    }

    if((flag == LEFT_JUSTIFY) && (length < width))
    {
        written += printfPad(width - length, false); //left justify flag, cant be zero pad at the same time
    }
    return written;
}

static int printfBase2(uint64_t x, uint8_t baseExponent, bool upperCase, uint32_t width, uint32_t precision, enum printfFlag flag)
{
    int written = 0;
    uint32_t length = findHighestPositionBase2(x, baseExponent);
    uint32_t numberLength = length;
    uint32_t precisionPad = 0;
    
    if(precision > length)
    {
        precisionPad = precision - length;
        length = precision; 
    }
    
    if((flag != LEFT_JUSTIFY) && (length < width))
    {
        written += printfPad(width - length, ZERO_PAD == flag);
    }

    if(DECIMAL == flag)
    {
        switch(baseExponent)
        {
            case 1:
                if(upperCase)
                    BootVgaPrintString("0B");
                else
                    BootVgaPrintString("0b");
                written += 2;
                break;
            case 3:
                BootVgaPrintChar('0');
                written++;
                break;
            case 4:
                if(upperCase)
                    BootVgaPrintString("0X");
                else
                    BootVgaPrintString("0x");
                written += 2;
                break;
        }
    }

    if(precisionPad)
    {
        written += printfPad(precisionPad, true); //always pad with zeros
    }
    
    while(1)
    {
        uint8_t val = (x >> ((uint64_t)numberLength * (uint64_t)baseExponent)) & ((uint64_t)(1 << baseExponent) - (uint64_t)1);
        if(val <= 9)
            BootVgaPrintChar((char)(val + '0'));
        else if(val <= 26)
        {
            val -= 10;
            if(upperCase)
                BootVgaPrintChar((char)(val + 'A')); 
            else
                BootVgaPrintChar((char)(val + 'a')); 
        }
        written++;
        if(0 == numberLength)
            break;
        numberLength--;
    }

    if((flag == LEFT_JUSTIFY) && (length < width))
    {
        written += printfPad(width - length, false); //left justify flag, cant be zero pad at the same time
    }
    return written;
}

static int printfFloat(double x, bool scientific, bool useShortest,
    bool upperCase, uint32_t width, bool precisionSpecified, 
    uint32_t precision, enum printfFlag flag)
{
    int64_t xInteger = 0;
    int written = 0;
    uint64_t power = 0;
    int16_t exponent = 0;
    uint32_t length = 0;

    if(__builtin_isinf(x))
    {
        length = 3;

        if((SIGN == flag) || (SIGN_REPLACE == flag) || (x < 0.0))
            length++;

        if(width > length)
            written += printfPad(width - length, ZERO_PAD == flag);
        
        if(x < 0.0)
            BootVgaPrintChar('-');
        else if((SIGN == flag) || (SIGN_REPLACE == flag))
            BootVgaPrintChar('+');
        
        if(upperCase)
            BootVgaPrintString("INF");
        else
            BootVgaPrintString("inf");
        
        return written + length;
    }

    if(__builtin_isnan(x))
    {
        if(width > 3)
            written += printfPad(width - 3, ZERO_PAD == flag);
        
        if(upperCase)
            BootVgaPrintString("NAN");
        else
            BootVgaPrintString("nan");
        
        return written + 3;
    }

    if(!precisionSpecified)
        precision = 6;

    if(useShortest)
    {
        double xs = dabs(doubleToScientific(x, &exponent));
        xs -= (double)((uint64_t)xs);
        uint32_t scientificLength = findHighestPosition(CmAbs(exponent), &power) + 3;
        if((xs != 0.0) || (DECIMAL == flag))
            scientificLength += 1 + precision;
        if((x < 0.0) || (SIGN == flag) || (SIGN_REPLACE == flag))
            scientificLength++;
        
        xInteger = CmAbs((int64_t)x);
        double xf = dabs(x) - (double)xInteger;
        uint32_t floatLength = findHighestPosition(xInteger, &power);
        if((xf != 0.0) || (DECIMAL == flag))
            floatLength += 1 + precision;
        if((x < 0.0) || (SIGN == flag) || (SIGN_REPLACE == flag))
            floatLength++;
        
        if(scientificLength < floatLength)
            scientific = true;
    }
    
    if(!scientific)
    {
        xInteger = (int64_t)x;
        length = findHighestPosition(CmAbs(xInteger), &power);
    }
    else
    {
        x = doubleToScientific(x, &exponent);
        xInteger = (int64_t)x;
        //3 because there is a digit before decimal and the "e+" or "e-" term
        length = 3 + findHighestPosition(CmAbs(exponent), &power);
    }

    x = dabs(x);
    x -= (double)(CmAbs(xInteger));
    
    x *= (double)CmPow10(precision);

    if((0.0 == x) && !precisionSpecified)
        precision = 0;

    if((DECIMAL == flag) || (precision > 0))
        length++;
    
    if((flag != LEFT_JUSTIFY) && ((length + precision) < width))
    {
        written += printfPad(width - (length + precision), ZERO_PAD == flag);
    }
    
    written += printfBase10(CmAbs(xInteger), xInteger > 0, 0, 0, flag);
    uint32_t digitsAfterDecimal = 0;
    if((DECIMAL == flag) || (precision > 0))
    {
        BootVgaPrintChar('.');
        written++;
        digitsAfterDecimal = printfBase10((uint64_t)x, true, 0, 0, NO_FLAG);
        written += digitsAfterDecimal;
    }
    if(digitsAfterDecimal < precision)
        written += printfPad(precision - digitsAfterDecimal, true);
    
    if(scientific)
    {
        if(upperCase)
            BootVgaPrintChar('E');
        else
            BootVgaPrintChar('e');

        written += printfBase10(CmAbs(exponent), exponent >= 0, 0, 0, SIGN);
    }

    if((flag == LEFT_JUSTIFY) && ((length + precision) < width))
    {
        written += printfPad(width - (length + precision), false); //left justify flag, cant be zero pad at the same time
    }
    return written;
}

int CmVprintf(const char *format, va_list args) 
{
	int written = 0;

	while(*format)
	{
		if('%' == *format)
		{
            uint8_t k = 1;
            enum printfFlag pFlag = NO_FLAG;
            //analyze flags
            switch(format[k])
            {
                case '-':
                    pFlag = LEFT_JUSTIFY;
                    break;
                case '+':
                    pFlag = SIGN;
                    break;
                case ' ':
                    pFlag = SIGN_REPLACE;
                    break;
                case '#':
                    pFlag = DECIMAL;
                    break;
                case '0':
                    pFlag = ZERO_PAD;
                    break;
                default:
                    k--;
                    break;
            }
            k++;
            //analyze width
            uint32_t width = 0;
            bool widthInFormat = false;
            if(format[k] == '*')
            {
                widthInFormat = true;
                k++;
            }
            else
            {
                while(format[k])
                {
                    if(!CmIsdigit(format[k]))
                        break;
                    width *= 10;
                    width += format[k] - '0';
                    k++;
                }
            }
            //analyze precision
            uint32_t precision = 0;
            bool precisionSpecified = false;
            bool precisionInFormat = false;
            if(format[k] == '.')
            {
                precisionSpecified = true;
                k++;
                if(format[k] == '*')
                {
                    precisionInFormat = true;
                    k++;
                }
                else
                {
                    while(format[k])
                    {
                        if(!CmIsdigit(format[k]))
                            break;
                        precision *= 10;
                        precision += format[k] - '0';
                        k++;
                    }
                }
            }
            //analyze length
            enum
            {
                L_DEFAULT,
                L_HH,
                L_H,
                L_L,
                L_LL,
                L_J,
                L_Z,
                L_T,
                L_LLL,
            } length = L_DEFAULT;

            switch(format[k])
            {
                case 'h':
                    if('h' == format[k + 1])
                    {
                        length = L_HH;
                        k++;
                    }
                    else
                        length = L_H;
                    break;
                case 'l':
                    if('l' == format[k + 1])
                    {
                        length = L_LL;
                        k++;
                    }
                    else
                        length = L_L;
                    break;
                case 'j':
                    length = L_J;
                    break;
                case 'z':
                    length = L_Z;
                    break;
                case 't':
                    length = L_T;
                    break;
                case 'L':
                    length = L_LLL;
                    break;
                default:
                    k--;
                    break;
            }
            k++;
            if(widthInFormat)
                width = va_arg(args, int);
            if(precisionInFormat)
                precision = va_arg(args, int);

            if(('i' == format[k]) || ('d' == format[k]))
            {
                int64_t x = 0;
                switch(length)
                {
                    case L_DEFAULT:
                    case L_HH:
                    case L_H:
                    default:
                        x = va_arg(args, int);
                        break;
                    case L_L:
                        x = va_arg(args, long int);
                        break;
                    case L_LL:
                        x = va_arg(args, long long int);
                        break;
                    case L_J:
                        x = va_arg(args, intmax_t);
                        break;
                    case L_Z:
                        x = va_arg(args, size_t);
                        break;
                    case L_T:
                        x = va_arg(args, ptrdiff_t);
                        break;
                }
                written += printfBase10(CmAbs(x), x >= 0, width, precision, pFlag);
            }
            else if(('u' == format[k]) || ('o' == format[k]) || ('x' == format[k]) || ('X' == format[k]) || ('b' == format[k]) || ('B' == format[k]))
            {
                uint64_t x = 0;
                switch(length)
                {
                    case L_DEFAULT:
                    case L_H:
                    case L_HH:
                    default:
                        x = va_arg(args, unsigned int);
                        break;
                    case L_L:
                        x = va_arg(args, unsigned long int);
                        break;
                    case L_LL:
                        x = va_arg(args, unsigned long long int);
                        break;
                    case L_J:
                        x = va_arg(args, uintmax_t);
                        break;
                    case L_Z:
                        x = va_arg(args, size_t);
                        break;
                    case L_T:
                        x = va_arg(args, ptrdiff_t);
                        break;
                }
                switch(format[k])
                {
                    case 'u':
                        written += printfBase10(x, true, width, precision, pFlag);
                        break;
                    case 'o':
                        written += printfBase2(x, 3, false, width, precision, pFlag);
                        break;
                    case 'x':
                    case 'X':
                        written += printfBase2(x, 4, 'X' == format[k], width, precision, pFlag);
                        break;
                    case 'b':
                    case 'B':
                        written += printfBase2(x, 1, 'B' == format[k], width, precision, pFlag);
                        break;
                }

            }
            else if(('f' == format[k]) || ('F' == format[k]) 
                || ('e' == format[k]) || ('E' == format[k]) 
                || ('g' == format[k]) || ('G' == format[k]) 
                || ('a' == format[k]) || ('A' == format[k])) //unsupported
            {
                double x = 0.0;
                switch(length)
                {
                    case L_DEFAULT:
                    default:
                        x = va_arg(args, double);
                        break;
                    case L_LLL:
                        x = va_arg(args, long double);
                        break;
                }
                switch(format[k])
                {
                    case 'f':
                    case 'F':
                        written += printfFloat(x, false, false, 'F' == format[k], width, precisionSpecified, precision, pFlag);
                        break;
                    case 'e':
                    case 'E':
                        written += printfFloat(x, true, false, 'E' == format[k], width, precisionSpecified, precision, pFlag);
                        break;
                    case 'g':
                    case 'G':
                        written += printfFloat(x, false, true, 'G' == format[k], width, precisionSpecified, precision, pFlag);
                        break;
                }     
            }
            else if('c' == format[k])
            {
                char x = va_arg(args, int);
                BootVgaPrintChar(x);
                written++;
            }
            else if('s' == format[k])
            {
                char *x = NULL;
                switch(length)
                {
                    case L_DEFAULT:
                    default:
                        x = va_arg(args, char*);
                        break;
                    case L_L:
                        x = (char*)va_arg(args, wchar_t*);
                        break;
                }
                uint32_t len = CmStrlen(x);
                if(LEFT_JUSTIFY == pFlag)
                {
                    if(precision)
                        BootVgaPrintStringN(x, precision);
                    else
                        BootVgaPrintString(x);
                    if(len < width)
                        written += printfPad(width - len, false);
                }
                else
                {
                    if(len < width)
                        written += printfPad(width - len, ZERO_PAD == pFlag);
                    if(precision)
                        BootVgaPrintStringN(x, precision);
                    else
                        BootVgaPrintString(x);
                }
                written += len;
            }
            else if('p' == format[k])
            {
                void *x = va_arg(args, void*);
                written += printfBase2((uint64_t)((uintptr_t)x), 4, false, width, precision, pFlag);
            }
            else if('n' == format[k])
            {
                switch(length)
                {
                    case L_DEFAULT:
                    default:
                        *va_arg(args, int*) = written;
                        break;
                    case L_HH:
                        *va_arg(args, char*) = written;
                        break;
                    case L_H:
                        *va_arg(args, short int*) = written;
                        break;
                    case L_L:
                        *va_arg(args, long int*) = written;
                        break;
                    case L_LL:
                        *va_arg(args, long long int*) = written;
                        break;
                    case L_J:
                        *va_arg(args, intmax_t*) = written;
                        break;
                    case L_Z:
                        *va_arg(args, size_t*) = written;
                        break;
                    case L_T:
                        *va_arg(args, ptrdiff_t*) = written;
                        break;
                }
            }
            else if('%' == format[k])
            {
                    BootVgaPrintChar('%');
                    written++;
                    break;
            }
            format += k;
		}
		else
        {
			BootVgaPrintChar(*format);
            written++;
        }
		format++;
	}

	return written;
}
