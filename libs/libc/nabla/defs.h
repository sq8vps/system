#ifndef NABLA_LIBC_DEFS_H
#define NABLA_LIBC_DEFS_H

#define __NABLA_LIBC_FILE_MAGIC 0x4E4CULL //"NF"
#define __NABLA_LIBC_FILE_FLAG_EOF 0x1ULL
#define __NABLA_LIBC_FILE_FLAG_ERROR 0x2ULL
#define __VALIDATE_STREAM(stream) (((NULL == (stream)) || (__NABLA_LIBC_FILE_MAGIC != ((stream)->flags >> 16))) ? (*__errno() = -EBADF, -1) : 0)

#define __WRITE true
#define __READ false

#endif