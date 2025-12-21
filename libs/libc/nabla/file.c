#include "utils.h"
#include "sys/file.h"

int close(int handle)
{
    reg_t r = __nabla_do_syscall(SYSCALL_CLOSE, (reg_t)handle, 0, 0, 0, 0);
    __LIBC_RETURN(int, r);
}

int open(const char * restrict filename, int mode, int flags)
{
    reg_t r = __nabla_do_syscall(SYSCALL_OPEN, (reg_t)filename, (reg_t)mode, (reg_t)flags, 0, 0);
    __LIBC_RETURN(int, r);
}

size_t read(int handle, void *buffer, size_t size, size_t offsetLo, size_t offsetHi)
{
    reg_t r = __nabla_do_syscall(SYSCALL_READ, (reg_t)handle, (reg_t)buffer, (reg_t)size, (reg_t)offsetLo, (reg_t)offsetHi);
    __LIBC_RETURN(size_t, r);
}

size_t write(int handle, const void *buffer, size_t size, size_t offsetLo, size_t offsetHi)
{
    reg_t r = __nabla_do_syscall(SYSCALL_WRITE, (reg_t)handle, (reg_t)buffer, (reg_t)size, (reg_t)offsetLo, (reg_t)offsetHi);
    __LIBC_RETURN(size_t, r);
}