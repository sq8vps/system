#include "utils.h"
#include "sys/mmap.h"

void *mmap(void *addr, size_t length, int flags, int fd, size_t offset)
{
    reg_t m = __nabla_do_syscall(SYSCALL_MMAP, (reg_t)addr, length, flags, fd, offset);
    __LIBC_RETURN(void*, m);
}

void *exmmap(void *addr, size_t length, int flags, int fd, const struct exmmap_params *const params)
{
    reg_t m = __nabla_do_syscall(SYSCALL_EXMMAP, (reg_t)addr, length, flags, fd, (reg_t)params);
    __LIBC_RETURN(void*, m);
}

int munmap(void *addr, size_t length)
{
    reg_t m = __nabla_do_syscall(SYSCALL_MUNMAP, (reg_t)addr, length, 0, 0, 0);
    __LIBC_RETURN(int, m);
}