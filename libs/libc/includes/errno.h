#ifndef NABLA_LIBC_ERRNO_H
#define NABLA_LIBC_ERRNO_H

#define ENOERR 0 /* No error */
#define EPERM 1 /* Operation not permitted */
#define ENOENT 2 /* No such file or directory */
#define ESRCH 3 /* No such process */
#define EINTR 4 /* Interrupted system call */
#define EIO 5 /* I/O error */
#define ENXIO 6 /* No such device or address */
#define E2BIG 7 /* Argument list too long */
#define ENOEXEC 8 /* Exec format error */
#define EBADF 9 /* Bad file descriptor */
#define ECHILD 10 /* No child processes */
#define EAGAIN 11 /* Try again */
#define ENOMEM 12 /* Out of memory */
#define EBUSY 16 /* Device or resource busy */
#define EEXIST 17 /* File exists */
#define ENODEV 19 /* No such device */
#define EINVAL 22 /* Invalid argument */
#define EROFS 30 /* Read-only file system */
#define EDOM 33 /**< Math argument out of domain */ 
#define ERANGE 34 /**< Math result not representable */
#define ENOSYS 38 /* Function not implemented */
#define ENOSUP 41 /* Operation not supported */
#define EALIGN 58 /* Bad alignment */
#define EILSEQ 84 /**< Illegal byte sequence */
#define ETIMEOUT 133 /* Operation timed out */
#define ECORRUPT 134 /* Data corrupted */
#define ECLOSED 135 /* File closed */
#define EINCMPLT 136 /* Operation finished with incomplete data */
#define EUNKNOWN 137 /*< Unknown error */

int *__errno(void);
#define errno (*__errno())

#endif