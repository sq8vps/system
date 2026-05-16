#include "state.h"
#include "utils.h"
#include "ke/task/task.h"

struct _nabla_libc_thread_state *_nabla_get_tls(void)
{
    return __nabla_get_tls();
}

void _nabla_set_tls(struct _nabla_libc_thread_state *tls)
{
    ApiSetThreadLocalStorage(tls);
}

int *__errno(void)
{
    return &(_nabla_get_tls()->errno);
}
