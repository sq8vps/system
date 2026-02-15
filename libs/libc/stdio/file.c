#include "stdio.h"
#include "io/fs/fs.h"
#include "nabla/nabla.h"
#include "nabla/utils.h"
#include "errno.h"
#include "stdlib.h"
#include "nabla/defs.h"

FILE *stdout = NULL;
FILE *stdin = NULL;
FILE *stderr = NULL;

static FILE* __create_fd(int handle)
{
    FILE *file = calloc(1, sizeof(*file));
    if(NULL == file)
    {
        __LIBC_RETURN_ERRNO(ENOMEM, NULL);
    }

    file->handle = handle;
    file->flags = __NABLA_LIBC_FILE_MAGIC << 16;
    file->buffer_base = malloc(BUFSIZ);
    file->buffer_size = BUFSIZ;
    file->buffer_ptr = file->buffer_base;
    file->buffer_end = file->buffer_base;
    file->buffer_mode = _IOLBF;
    file->pushback = EOF;
    if(NULL == file->buffer_base)
    {
        free(file);
        __LIBC_RETURN_ERRNO(ENOMEM, NULL);
    }

    return file;
}

int fclose(FILE *stream)
{
    if(NULL == stream)
    {
        __LIBC_RETURN_ERRNO(EBADF, EOF);
    }
    STATUS status = ApiCloseFile(stream->handle);
    stream->flags = 0;
    stream->handle = -1;
    free(stream->buffer_base);
    free(stream);
    if(OK != status)
        __LIBC_RETURN_STATUS(status, EOF);
    
    return 0;
}

FILE *fopen(const char * restrict filename, const char * restrict mode)
{
    STATUS status = OK;
    IoFileOpenMode fmode = 0;
    IoFileFlags fflags = IO_FILE_FLAG_SHARED;
    FILE *file = NULL;
    int handle = -1;

    if((NULL == filename) || (NULL == mode))
    {
        __LIBC_RETURN_ERRNO(EINVAL, NULL);
    }

    switch(mode[0])
    {
        case 'r':
            fmode |= IO_FILE_READ;
            break;
        case 'w':
            fmode |= IO_FILE_WRITE | IO_FILE_REPLACE | IO_FILE_CREATE;
            break;
        case 'a':
            fmode |= IO_FILE_APPEND | IO_FILE_CREATE;
            break;
        default:
            __LIBC_RETURN_ERRNO(EINVAL, NULL);
    }

    switch(mode[1])
    {
        case 'b':
            break;
        case 'x':
            fflags &= ~IO_FILE_FLAG_SHARED;
            break;
        case '+':
            if('r' == mode[0])
                fmode |= IO_FILE_WRITE;
            else if('w' == mode[0])
                fmode |= IO_FILE_READ;
            else if('a' == mode[0])
                fmode |= IO_FILE_READ;
            break;
        case '\0':
            break;
        default:
            __LIBC_RETURN_ERRNO(EINVAL, NULL);
    }

    if(('\0' != mode[1]) && ('\0' != mode[2]))
    {
        switch(mode[2])
        {
            case 'b':
                break;
            case 'x':
                fflags &= ~IO_FILE_FLAG_SHARED;
                break;
            case '+':
                if('r' == mode[0])
                    fmode |= IO_FILE_WRITE;
                else if('w' == mode[0])
                    fmode |= IO_FILE_READ;
                else if('a' == mode[0])
                    fmode |= IO_FILE_READ;
                break;
            default:
                __LIBC_RETURN_ERRNO(EINVAL, NULL);
        }

        if('\0' != mode[3])
        {
            switch(mode[3])
            {
                case 'x':
                    fflags &= ~IO_FILE_FLAG_SHARED;
                    break;
                default:
                    __LIBC_RETURN_ERRNO(EINVAL, NULL);
            }
        }
    }

    status = ApiOpenFile(filename, fmode, fflags, &handle);
    if(OK != status)
    {
        __LIBC_RETURN_STATUS(status, NULL);
    }

    file = __create_fd(handle);
    if(NULL == file)
    {
        ApiCloseFile(handle);
        return NULL;
    }
    
    return file;
}

void __nabla_libc_initialize_stdio(void)
{
    stdin = __create_fd(__STDIN_HANDLE);
    stdout = __create_fd(__STDOUT_HANDLE);
    stderr = __create_fd(__STDERR_HANDLE);
}