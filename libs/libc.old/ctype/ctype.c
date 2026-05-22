#include "ctype.h"

int isalnum(int c)
{
    return (isalpha(c) || isdigit(c));
}

int isalpha(int c)
{
    if(isupper(c) || islower(c))
        return 1;
    return 0;
}

int isblank(int c)
{
    return ((' ' == c) || ('\t' == c));
}

int iscntrl(int c)
{
    return (((c >= 0x00) && (c <= 0x1F)) || (0x7F == c));
}

int isdigit(int c)
{
    return ((c >= '0') && (c <= '9'));
}

int isgraph(int c)
{
    return ((c > 0x20) && (c <= 0x7E));
}

int islower(int c)
{
    return ((c >= 'a') && (c <= 'z'));
}

int isprint(int c)
{
    return ((c >= 0x20) && (c <= 0x7E));
}

int ispunct(int c)
{
    return (!isspace(c) && !isalnum(c));
}

int isspace(int c)
{
    return ((' ' == c) || ('\f' == c) || ('\n' == c) || ('\r' == c) || ('\t' == c) || ('\v' == c));
}

int isupper(int c)
{
    return ((c >= 'A') && (c <= 'Z'));
}

int isxdigit(int c)
{
    return (((c >= '0') && (c <= '9')) 
            || ((c >= 'a') && (c <= 'f')) 
            || ((c >= 'A') && (c <= 'F'))
            );
}

int tolower(int c)
{
    if(isupper(c))
        return c + ('a' - 'A');
    else
        return c;
}

int toupper(int c)
{
    if(islower(c))
        return c - ('a' - 'A');
    else
        return c;
}