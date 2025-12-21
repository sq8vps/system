#include "stdlib.h"

int abs(int j)
{
    return (j < 0) ? -j : j;
}

long int labs(long int j)
{
    return (j < 0l) ? -j : j;
}

long long int llabs(long long int j)
{
    return (j < 0ll) ? -j : j;
}

div_t div(int numer, int denom)
{
    div_t result;
    result.quotient = numer / denom;
    result.remainder = numer % denom;
    return result;
}

ldiv_t ldiv(long int numer, long int denom)
{
    ldiv_t result;
    result.quotient = numer / denom;
    result.remainder = numer % denom;
    return result;
}

lldiv_t lldiv(long long int numer, long long int denom)
{
    lldiv_t result;
    result.quotient = numer / denom;
    result.remainder = numer % denom;
    return result;
}