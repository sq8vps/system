#ifndef NABLA_LIBC_UTILS_H
#define NABLA_LIBC_UTILS_H

#include <stdint.h>

typedef int32_t reg_t;

reg_t __nabla_do_syscall(reg_t code, reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5);

int __nabla_kernel_status_to_errno(reg_t status);

char *__nabla_libc_strerror(int errno);

void *__nabla_get_tls(void);

int *__errno(void);

#define __LIBC_RETURN(type, value) return ((value) < 0 ? (*__errno() = __nabla_kernel_status_to_errno(value), (type)(-1)) : (type)(value))

#define __ALIGN_MASK(x, mask) (((x) + (mask)) &~ (mask))
#define _ALIGN_UP(x, alignment) __ALIGN_MASK(x, (typeof(x))(alignment) - 1)
#define _ALIGN_DOWN(x, alignment) ((x) & ~((typeof(val))(align) - 1))

#endif