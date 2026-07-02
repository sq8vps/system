#include <sys/times.h>
#include <sys/time.h>
#include "helpers.h"

clock_t _times(struct tms *buf)
{
    errno = ENOSYS;
    return (clock_t)(-1);
    //TODO: implement times()
}

int _gettimeofday(struct timeval *ptimeval, void *ptimezone)
{
    errno = ENOSYS;
    return -1;
    //TODO: implement gettimeofday()
}