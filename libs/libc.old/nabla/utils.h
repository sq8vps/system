#ifndef NABLA_LIBC_UTILS_H
#define NABLA_LIBC_UTILS_H

#include "defines.h"

int __nabla_kernel_status_to_errno(STATUS status);

char *__nabla_libc_strerror(int errno);

void *__nabla_get_tls(void);

int *__errno(void);

#define __LIBC_RETURN_ERRNO(err, toReturn) return (*__errno() = (err), (toReturn))
#define __LIBC_RETURN_STATUS(status, toReturn) __LIBC_RETURN_ERRNO(__nabla_kernel_status_to_errno(status), (toReturn))

#endif