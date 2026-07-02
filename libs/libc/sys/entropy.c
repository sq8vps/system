#include <stddef.h>
#include <sys/errno.h>

int _getentropy(void *buffer, size_t length)
{
    errno = ENOSYS;
    return -1;
    //TODO: implement getentropy()
}