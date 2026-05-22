#include "sys/signal.h"
#include "errno.h"

int sigprocmask(int how, const sigset_t *restrict set, sigset_t *restrict oldset)
{
    //TODO: implement sigprocmask()
    errno = ENOSYS;
    return -1;
}