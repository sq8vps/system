#include "stdio.h"
#include "sys/file.h"
#include "nabla/nabla.h"
#include "errno.h"
#include "stdlib.h"
#include "nabla/defs.h"

int fclose(FILE *stream)
{
    if(NULL == stream)
    {
        *__errno() = -EBADF;
        return EOF;
    }
    int status = close(stream->handle);
    stream->flags = 0;
    stream->handle = -1;
    free(stream->buffer_base);
    free(stream);
    if(status < 0)
    {
        return EOF;
    }
    else    
        return -ENOERR;
}

FILE *fopen(const char * restrict filename, const char * restrict mode)
{
    int fmode = 0;
    int fflags = FILE_FLAG_SHARED;
    FILE *file = NULL;
    int handle = -1;

    if((NULL == filename) || (NULL == mode))
    {
        *__errno() = -EINVAL;
        return NULL;
    }

    switch(mode[0])
    {
        case 'r':
            fmode |= FILE_READ;
            break;
        case 'w':
            fmode |= FILE_WRITE | FILE_REPLACE | FILE_CREATE;
            break;
        case 'a':
            fmode |= FILE_APPEND | FILE_CREATE;
            break;
        default:
            *__errno() = -EINVAL;
            return NULL;
    }

    switch(mode[1])
    {
        case 'b':
            break;
        case 'x':
            fflags &= ~FILE_FLAG_SHARED;
            break;
        case '+':
            if('r' == mode[0])
                fmode |= FILE_WRITE;
            else if('w' == mode[0])
                fmode |= FILE_READ;
            else if('a' == mode[0])
                fmode |= FILE_READ;
            break;
        case '\0':
            break;
        default:
            *__errno() = -EINVAL;
            return NULL;
    }

    if(('\0' != mode[1]) && ('\0' != mode[2]))
    {
        switch(mode[2])
        {
            case 'b':
                break;
            case 'x':
                fflags &= ~FILE_FLAG_SHARED;
                break;
            case '+':
                if('r' == mode[0])
                    fmode |= FILE_WRITE;
                else if('w' == mode[0])
                    fmode |= FILE_READ;
                else if('a' == mode[0])
                    fmode |= FILE_READ;
                break;
            default:
                *__errno() = -EINVAL;
                return NULL;
        }

        if('\0' != mode[3])
        {
            switch(mode[3])
            {
                case 'x':
                    fflags &= ~FILE_FLAG_SHARED;
                    break;
                default:
                    *__errno() = -EINVAL;
                    return NULL;
            }
        }
    }

    file = calloc(1, sizeof(*file));
    if(NULL == file)
    {
        *__errno() = -ENOMEM;
        return NULL;
    }

    handle = open(filename, fmode, fflags);
    if(handle < 0)
    {
        free(file);
        return NULL;
    }

    file->handle = handle;
    file->flags = __NABLA_LIBC_FILE_MAGIC << 16;
    file->buffer_base = malloc(__NABLA_LIBC_FILE_BUFFER_SIZE);
    file->buffer_size = __NABLA_LIBC_FILE_BUFFER_SIZE;
    file->buffer_ptr = file->buffer_base;
    file->buffer_end = file->buffer_base;
    file->buffer_mode = _IOLBF;
    file->pushback = EOF;
    if(NULL == file->buffer_base)
    {
        close(handle);
        free(file);
        return NULL;
    }
    return file;
}