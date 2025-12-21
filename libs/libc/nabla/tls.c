#include "state.h"
#include "utils.h"
#include "sys/mmap.h"

struct _nabla_libc_thread_state *_nabla_get_tls(void)
{
    return __nabla_get_tls();
}

void _nabla_set_tls(struct _nabla_libc_thread_state *tls)
{
    __nabla_do_syscall(SYSCALL_SET_TLS, (reg_t)tls, 0, 0, 0, 0);
}

int *__errno(void)
{
    return &(_nabla_get_tls()->errno);
}
