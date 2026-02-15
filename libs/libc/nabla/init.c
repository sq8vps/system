#include "nabla.h"
#include "state.h"
#include "ke/sys/syscall.h"

void __nabla_libc_initialize_stdio(void);

void __nabla_libc_initialize(void)
{
    struct _nabla_libc_thread_state dummyTls = {.self = &dummyTls, .errno = 0, .heap_state = NULL};
    _nabla_set_tls(&dummyTls);

    struct _nabla_libc_thread_state *tls = _nabla_create_tls();
    if(NULL == tls)
    {
        //TODO: do something with it
    }
    _nabla_set_tls(tls);

    uint64_t page_size = 0;
    if(OK != ApiGetSystemConfig(API_SYSTEM_CONFIG_PAGE_SIZE, &page_size))
    {
        //TODO: do something with it
    }
    tls->config.page_size = (size_t)page_size;

    __nabla_libc_initialize_stdio();
}

int main(int argc, char **argv, char **envp);

[[noreturn]] void _start(int argc, char **argv, char **envp)
{
    __nabla_libc_initialize();
    int code = main(argc, argv, envp);
    (void)code;
    //TODO: exit program

    while(1)
        ;
}