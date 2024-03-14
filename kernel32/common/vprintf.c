#include "defines.h"
#include "vprintf.h"
#include "common.h"
#include "sdrv/bootvga/bootvga.h"
#include "mm/heap.h"

static uint8_t _FindHighestPositionBase10(uint64_t n, uint64_t *power)
{
    *power = 1;
    uint8_t pos = 0;
    do
    {
        n /= 10;
        pos++;
        (*power) *= 10;
    } while (n != 0);

    (*power) /= 10;
    return pos;
}

static uint8_t _FindHighestPositionBase2(uint64_t n, uint8_t baseExponent)
{
    if (0 == baseExponent)
        return 0;
    uint64_t pos = 0;
    do
    {
        n >>= baseExponent;
        pos++;
    } while (n != 0);
    return pos;
}

static int _PrintPad(struct VPrintfConfig *config, uint32_t count, bool zeroPad, size_t alreadyWritten)
{
    if (config->toFile)
    {
        char *t = MmAllocateKernelHeap(count);
        if (NULL != t)
        {
            CmMemset(t, zeroPad ? '0' : ' ', count);
            BootVgaPrintStringN(t, count);
            MmFreeKernelHeap(t);
            return count;
        }
        return 0;
    }
    else
    {
        CmMemset(&(config->buffer[alreadyWritten]), zeroPad ? '0' : ' ', count);
        return count;
    }
}

enum _PrintFlag
{
    NO_FLAG,
    LEFT_JUSTIFY,
    SIGN,
    SIGN_REPLACE,
    DECIMAL,
    ZERO_PAD,
};

static int _PrintNumber(struct VPrintfConfig *config, uint64_t x, bool sign, uint32_t width,
                        bool usePrecision, uint32_t precision, enum _PrintFlag flag,
                        uint8_t baseExponent, bool upperCase, size_t alreadyWritten)
{
    int written = 0;
    uint64_t power = 0;
    uint32_t length = 0;
    uint32_t leftWidthPadding = 0, leftPrecisionPadding = 0, rightPadding = 0, prefixLength = 0;

    if (0 == baseExponent)
        length = _FindHighestPositionBase10(x, &power);
    else
    {
        length = _FindHighestPositionBase2(x, baseExponent);
        power = (length - 1) * baseExponent;
    }

    size_t remainingSpace = SIZE_MAX;
    if (config->useMax)
        remainingSpace = config->max - alreadyWritten;

    if (length < precision)
    {
        leftPrecisionPadding = precision - length; // zero-pad from the left if precision is higher
    }

    if (!(usePrecision && (0 == precision) && (0 == x)))
    {
        if (!sign || (SIGN == flag) || (SIGN_REPLACE == flag))
            prefixLength = 1; // sign is going to be prepended
        else if (DECIMAL == flag)
        {
            switch (baseExponent)
            {
            case 1: // binary
                prefixLength = 2;
                break;
            case 3: // octal
                prefixLength = 1;
                break;
            case 4: // hex
                prefixLength = 2;
                break;
            default:
                break;
            }
        }
    }

    if (width > (length + leftPrecisionPadding + prefixLength)) // additional padding to reach specified width
    {
        if (LEFT_JUSTIFY == flag) // right padding
        {
            rightPadding = width - (length + leftPrecisionPadding + prefixLength);
        }
        else // standard left padding
        {
            leftWidthPadding = (width - (length + leftPrecisionPadding + prefixLength));
        }
    }

    char *buf = NULL;
    if (config->toFile)
    {
        buf = MmAllocateKernelHeap(length + leftPrecisionPadding + leftWidthPadding + rightPadding + prefixLength);
        if (NULL == buf)
            return 0;
    }

    if ((0 != leftWidthPadding) && (0 != remainingSpace))
    {
        // left padding might be specified to use zeros instead of spaces
        if (config->useMax && (leftWidthPadding > remainingSpace))
            leftWidthPadding = remainingSpace;

        if (config->toFile)
            CmMemset(buf, (ZERO_PAD == flag) ? '0' : ' ', leftWidthPadding);
        else
            CmMemset(&config->buffer[alreadyWritten + written], (ZERO_PAD == flag) ? '0' : ' ', leftWidthPadding);
        written += leftWidthPadding;
        remainingSpace -= leftWidthPadding;
    }

    if ((0 != prefixLength) && (0 != remainingSpace))
    {
        if ((0 == baseExponent) || (DECIMAL != flag)) // decimal system or 2^n system but without prefix (0x, 0, 0B etc.)
        {
            if (config->toFile)
                buf[written] = sign ? ((SIGN_REPLACE == flag) ? ' ' : '+') : '-';
            else
                config->buffer[alreadyWritten + written] = sign ? ((SIGN_REPLACE == flag) ? ' ' : '+') : '-';

            written++;
            remainingSpace--;
        }
        else // 2^n base with prefix
        {
            // each system uses '0' at the beginning
            if (config->toFile)
                buf[written] = '0';
            else
                config->buffer[alreadyWritten + written] = '0';

            written++;
            remainingSpace--;
            if (0 != remainingSpace)
            {
                if ((1 == baseExponent) || (4 == baseExponent)) // bin or hex
                {
                    char symbol;
                    if (1 == baseExponent)
                        symbol = upperCase ? 'B' : 'b';
                    else
                        symbol = upperCase ? 'X' : 'x';
                    if (config->toFile)
                        buf[written] = symbol;
                    else
                        config->buffer[alreadyWritten + written] = symbol;

                    written++;
                    remainingSpace--;
                }
            }
        }
    }

    if ((0 != leftPrecisionPadding) && (0 != remainingSpace))
    {
        if (config->useMax && (leftPrecisionPadding > remainingSpace))
            leftPrecisionPadding = remainingSpace;

        if (config->toFile)
            CmMemset(&buf[written], '0', leftPrecisionPadding);
        else
            CmMemset(&config->buffer[alreadyWritten + written], '0', leftPrecisionPadding);
        written += leftPrecisionPadding;
        remainingSpace -= leftPrecisionPadding;
    }

    if ((0 != remainingSpace) && !(usePrecision && (0 == precision) && (0 == x)))
    {
        if (config->useMax && (length > remainingSpace))
            length = remainingSpace;

        if (0 == baseExponent) // decimal
        {
            for (uint32_t i = 0; i < length; i++)
            {
                uint8_t digit = x / power;
                if (config->toFile)
                    buf[written] = digit + '0';
                else
                    config->buffer[alreadyWritten + written] = digit + '0';
                x -= (digit * power);
                power /= 10;
                written++;
                remainingSpace--;
            }
        }
        else // 2^n
        {
            for (uint32_t i = 0; i < length; i++)
            {
                uint8_t val = (x >> power) & (((uint64_t)1 << (uint64_t)baseExponent) - (uint64_t)1);
                char symbol;
                if (val <= 9)
                    symbol = val + '0';
                else if (val <= 26)
                {
                    val -= 10;
                    if (upperCase)
                        symbol = val + 'A';
                    else
                        symbol = val + 'a';
                }
                if (config->toFile)
                    buf[written] = symbol;
                else
                    config->buffer[alreadyWritten + written] = symbol;

                power -= baseExponent;
                written++;
                remainingSpace--;
            }
        }
    }

    if ((0 != rightPadding) && (0 != remainingSpace))
    {
        // right padding (left justification) is always done with spaces
        if (config->useMax && (rightPadding > remainingSpace))
            rightPadding = remainingSpace;

        if (config->toFile)
            CmMemset(&buf[written], ' ', rightPadding);
        else
            CmMemset(&config->buffer[alreadyWritten + written], ' ', rightPadding);
        written += rightPadding;
        remainingSpace -= rightPadding;
    }

    if (config->toFile)
    {
        BootVgaPrintStringN(buf, written);
        MmFreeKernelHeap(buf);
    }

    return written;
}

int CmVprintf(struct VPrintfConfig config, const char *format, va_list args)
{
    int written = 0;

    while (*format)
    {
        if ('%' == *format)
        {
            uint8_t k = 1;
            enum _PrintFlag pFlag = NO_FLAG;
            // analyze flags
            switch (format[k])
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
            // analyze width
            uint32_t width = 0;
            bool widthInFormat = false;
            if (format[k] == '*')
            {
                widthInFormat = true;
                k++;
            }
            else
            {
                while (format[k])
                {
                    if (!CmIsdigit(format[k]))
                        break;
                    width *= 10;
                    width += format[k] - '0';
                    k++;
                }
            }
            // analyze precision
            uint32_t precision = 0;
            bool precisionSpecified = false;
            bool precisionInFormat = false;
            if (format[k] == '.')
            {
                precisionSpecified = true;
                k++;
                if (format[k] == '*')
                {
                    precisionInFormat = true;
                    k++;
                }
                else
                {
                    while (format[k])
                    {
                        if (!CmIsdigit(format[k]))
                            break;
                        precision *= 10;
                        precision += format[k] - '0';
                        k++;
                    }
                }
            }
            // analyze length
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

            switch (format[k])
            {
            case 'h':
                if ('h' == format[k + 1])
                {
                    length = L_HH;
                    k++;
                }
                else
                    length = L_H;
                break;
            case 'l':
                if ('l' == format[k + 1])
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
            if (widthInFormat)
                width = va_arg(args, int);
            if (precisionInFormat)
                precision = va_arg(args, int);

            if (('i' == format[k]) || ('d' == format[k]))
            {
                int64_t x = 0;
                switch (length)
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
                written += _PrintNumber(&config, CmAbs(x), x >= 0, width, precisionSpecified, precision, pFlag, 0, false, written);
            }
            else if (('u' == format[k]) || ('o' == format[k]) || ('x' == format[k]) || ('X' == format[k]) || ('b' == format[k]) || ('B' == format[k]))
            {
                uint64_t x = 0;
                switch (length)
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
                switch (format[k])
                {
                case 'u':
                    written += _PrintNumber(&config, x, true, width, precisionSpecified, precision, pFlag, 0, false, written);
                    break;
                case 'o':
                    written += _PrintNumber(&config, x, true, width, precisionSpecified, precision, pFlag, 3, false, written);
                    break;
                case 'x':
                case 'X':
                    written += _PrintNumber(&config, x, true, width, precisionSpecified, precision, pFlag, 4, 'X' == format[k], written);
                    break;
                case 'b':
                case 'B':
                    written += _PrintNumber(&config, x, true, width, precisionSpecified, precision, pFlag, 1, 'B' == format[k], written);
                    break;
                }
            }
            else if ('c' == format[k])
            {
                char x = va_arg(args, int);
                if (config.useMax && (written < config.max))
                {
                    if (config.toFile)
                    {
                        BootVgaPrintChar(x);
                    }
                    else
                    {
                        config.buffer[written] = x;
                    }
                    written++;
                }
            }
            else if ('s' == format[k])
            {
                char *x = NULL;
                switch (length)
                {
                case L_DEFAULT:
                default:
                    x = va_arg(args, char *);
                    break;
                case L_L:
                    x = (char *)va_arg(args, wchar_t *);
                    break;
                }
                uint32_t len = CmStrlen(x);
                size_t toWrite = len;
                if (precision)
                    toWrite = (len > precision) ? precision : len;

                if (config.useMax)
                {
                    if ((config.max - written) < toWrite)
                        toWrite = config.max - written;
                }

                if (LEFT_JUSTIFY != pFlag)
                {
                    if (toWrite < width)
                    {
                        size_t padLength = width - toWrite;
                        if (config.useMax && ((config.max - written) < padLength))
                            padLength = config.max - written;

                        written += _PrintPad(&config, padLength, ZERO_PAD == pFlag, written);
                    }
                }

                if (config.toFile)
                    BootVgaPrintStringN(x, len);
                else
                    CmMemcpy(&(config.buffer[written]), x, toWrite);

                written += toWrite;

                if (LEFT_JUSTIFY == pFlag)
                {
                    if (toWrite < width)
                    {
                        size_t padLength = width - toWrite;
                        if (config.useMax && ((config.max - written) < padLength))
                            padLength = config.max - written;

                        written += _PrintPad(&config, padLength, false, written);
                    }
                }
            }
            else if ('p' == format[k])
            {
                void *x = va_arg(args, void *);
                written += _PrintNumber(&config, (uintptr_t)x, true, width, precisionSpecified, precision, pFlag, 4, false, written);
            }
            else if ('n' == format[k])
            {
                switch (length)
                {
                case L_DEFAULT:
                default:
                    *va_arg(args, int *) = written;
                    break;
                case L_HH:
                    *va_arg(args, char *) = written;
                    break;
                case L_H:
                    *va_arg(args, short int *) = written;
                    break;
                case L_L:
                    *va_arg(args, long int *) = written;
                    break;
                case L_LL:
                    *va_arg(args, long long int *) = written;
                    break;
                case L_J:
                    *va_arg(args, intmax_t *) = written;
                    break;
                case L_Z:
                    *va_arg(args, size_t *) = written;
                    break;
                case L_T:
                    *va_arg(args, ptrdiff_t *) = written;
                    break;
                }
            }
            else if ('%' == format[k])
            {
                if (config.useMax && (written < config.max))
                {
                    if (config.toFile)
                        BootVgaPrintChar('%');
                    else
                        config.buffer[written] = '%';
                }
                break;
            }
            format += k;
        }
        else
        {
            const char *last = format;
            while ((*last != '%') && (*last != '\0'))
            {
                last++;
            }

            size_t len = last - format;

            if (config.useMax && ((config.max - written) < len))
                len = config.max - written;

            if (config.toFile)
                BootVgaPrintStringN((char *)format, len);
            else
                CmMemcpy(&(config.buffer[written]), format, len);

            written += len;
            format = last - 1;
        }
        format++;
    }

    if (!config.toFile)
        config.buffer[written++] = '\0';

    return written;
}
