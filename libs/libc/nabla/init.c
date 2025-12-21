#include "nabla.h"
#include "sys/mmap.h"
#include "state.h"

void __nabla_libc_initialize(void)
{
    void *tls = _nabla_create_tls();
    if(NULL == tls)
    {
        //TODO: do something with it
    }
    _nabla_set_tls(tls);
}