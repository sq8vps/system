#include "stdio.h"
#include "nabla/utils.h"
#include "io/fs/fs.h"
#include "errno.h"
#include "nabla/defs.h"
#include "stdlib.h"
#include "string.h"

static size_t __read_write(void * restrict ptr, size_t size, FILE * restrict stream, bool _write)
{
    STATUS status = OK;
    size_t actual = 0;
    if(0 == size)
        return 0;

    if(NULL == ptr)
        __LIBC_RETURN_ERRNO(-EINVAL, (size_t)(-1));
    
    if(__VALIDATE_STREAM(stream) < 0)
        return (size_t)(-1);

    if(__WRITE == _write)
    {
        status = ApiWriteFileSync(stream->handle, ptr, size, stream->pos, &actual);
    }
    else
    {
        status = ApiReadFileSync(stream->handle, ptr, size, stream->pos, &actual);
    }

    if(OK != status)
        __LIBC_RETURN_STATUS(status, -1);
    else
        return actual;
}

int fgetc(FILE *stream)
{
    if(__VALIDATE_STREAM(stream) < 0)
        return EOF;

    if(stream->flags & __NABLA_LIBC_FILE_FLAG_EOF)
        return EOF;

    if(EOF != stream->pushback)
    {
        int c = stream->pushback;
        stream->pushback = EOF;
        return c;
    }

    if(_IONBF != stream->buffer_mode)
    {
        if(stream->buffer_ptr != stream->buffer_end)
        {
            return (unsigned char)*(stream->buffer_ptr++);
        }
        else
        {
            stream->pos += (stream->buffer_end - stream->buffer_base);
            stream->buffer_ptr = stream->buffer_base;
            stream->buffer_end = stream->buffer_base;
            size_t count = __read_write(stream->buffer_base, stream->buffer_size, stream, __READ);
            if((size_t)(-1) == count)
            {
                stream->flags |= __NABLA_LIBC_FILE_FLAG_ERROR;
                return EOF;
            }
            if(0 == count)
            {
                stream->flags |= __NABLA_LIBC_FILE_FLAG_EOF;
                return EOF;
            }
            
            stream->buffer_end += count;
            return (unsigned char)*(stream->buffer_ptr++);
        }
    }
    else //no buffering
    {
        unsigned char c;
        size_t count = __read_write(&c, 1, stream, __READ);
        if((size_t)(-1) == count)
        {
            stream->flags |= __NABLA_LIBC_FILE_FLAG_ERROR;
            return EOF;
        }
        if(0 == count)
        {
            stream->flags |= __NABLA_LIBC_FILE_FLAG_EOF;
            return EOF;
        }
        ++stream->pos;
        return c;
    }
}

char *fgets(char * restrict s, int n, FILE * restrict stream)
{
    int written = 0;

    if(__VALIDATE_STREAM(stream) < 0)
        return NULL;

    if(n <= 0)
        return s;

    if(EOF != stream->pushback)
    {
        if(1 == n)
        {
            *s = '\0';
            return s;
        }

        s[written++] = (unsigned char)stream->pushback;
        stream->pushback = EOF;
        if(0 == --n)
        {
            s[written++] = '\0';
            return s;
        }
    }

    if(stream->flags & __NABLA_LIBC_FILE_FLAG_EOF)
        return NULL;

    if(_IONBF != stream->buffer_mode)
    {
        if(stream->buffer_ptr != stream->buffer_end)
        {
            size_t count = ((stream->buffer_end - stream->buffer_ptr) < n) ? 
                (stream->buffer_end - stream->buffer_ptr) : n;

            for(size_t i = 0; i < count; i++)
            {
                s[written++] = *(stream->buffer_ptr++);
                if(('\n' == s[written - 1]) || (written == (n - 1)))
                {
                    s[written] = '\0';
                    return s;
                }
            }
        }
        
        if(written != n)
        {
            while(1)
            {
                stream->pos += (stream->buffer_end - stream->buffer_base);
                stream->buffer_ptr = stream->buffer_base;
                stream->buffer_end = stream->buffer_base;
                size_t count = __read_write(stream->buffer_base, stream->buffer_size, stream, __READ);
                if((size_t)(-1) == count)
                {
                    stream->flags |= __NABLA_LIBC_FILE_FLAG_ERROR;
                    return NULL;
                }
                if(0 == count)
                {
                    s[written] = '\0';
                    stream->flags |= __NABLA_LIBC_FILE_FLAG_EOF;
                    return s;
                }

                stream->buffer_end += count;

                if(count > (size_t)(n - written))
                    count = (size_t)(n - written);

                for(size_t i = 0; i < count; i++)
                {
                    s[written++] = *(stream->buffer_ptr++);
                    if(('\n' == s[written - 1]) || (written == (n - 1)))
                    {
                        s[written] = '\0';
                        return s;
                    }
                }
            }
        }
    }
    else //no buffering
    {
        char *buf = malloc(n);
        char *end = NULL;
        if(NULL == buf)
            __LIBC_RETURN_ERRNO(-ENOMEM, NULL);

        size_t count = __read_write(buf, n - 1, stream, __READ);
        if((size_t)(-1) == count)
        {
            free(buf);
            stream->flags |= __NABLA_LIBC_FILE_FLAG_ERROR;
            return NULL;
        }
        if(0 == count)
            stream->flags |= __NABLA_LIBC_FILE_FLAG_EOF;

        buf[count] = '\0';
        for(size_t i = 0; i < count; i++)
        {
            if('\n' == buf[i])
            {
                buf[i + 1] = '\0';
                end = buf + i + 1;
                break;
            }
        }
        memcpy(s, buf, (size_t)(end - buf));
        stream->pos += (size_t)(end - buf);
        free(buf);
    }
    
    return s;
}

int fputc(int c, FILE *stream)
{
    if(__VALIDATE_STREAM(stream) < 0)
        return EOF;

    if(_IONBF != stream->buffer_mode)
    {
        *(stream->buffer_ptr++) = (unsigned char)c;
        ++stream->buffer_end;
        if((stream->buffer_end == (stream->buffer_base + stream->buffer_size)) || ((_IOLBF == stream->buffer_mode) && ('\n' == c)))
        {
            size_t written = __read_write(stream->buffer_base, stream->buffer_end - stream->buffer_base, stream, __WRITE);
            if((size_t)(-1) == written)
            {
                return EOF;
            }
            stream->pos += (stream->buffer_end - stream->buffer_base);
            stream->buffer_ptr = stream->buffer_base;
            stream->buffer_end = stream->buffer_base;
        }
        return c;
    }
    else //no buffering
    {
        size_t count = __read_write(&c, 1, stream, __WRITE);
        if(1 != count)
        {
            return EOF;
        }
        ++stream->pos;
        return c;
    }
}

int fputs(const char * restrict s, FILE * restrict stream)
{
    size_t len = 0;

    if(__VALIDATE_STREAM(stream) < 0)
        return EOF;

    if(NULL == s)
        __LIBC_RETURN_ERRNO(-EINVAL, EOF);

    len = strlen(s);
    if(0 == len)
        return 0;

    if(_IONBF != stream->buffer_mode)
    {
        size_t i = 0;
        while(1)
        {
            *(stream->buffer_ptr++) = s[i];
            ++stream->buffer_end;
            if((('\n' == s[i]) && (_IOLBF == stream->buffer_mode)) 
                || (stream->buffer_end == (stream->buffer_base + stream->buffer_size)))
            {
                size_t count = __read_write(stream->buffer_base, stream->buffer_end - stream->buffer_base, stream, __WRITE);
                if(count != (size_t)(stream->buffer_end - stream->buffer_base))
                {
                    return EOF;
                }
                stream->pos += (stream->buffer_end - stream->buffer_base);
                stream->buffer_ptr = stream->buffer_base;
                stream->buffer_end = stream->buffer_base;
            }
            ++i;
            if(len == i)
                return 0;
        }
    }
    else //no buffering
    {
        size_t count = __read_write((char*)s, len, stream, __WRITE);
        if(count != len)
        {
            return EOF;
        }
        stream->pos += count;
        return 0;
    }
}

int getc(FILE *stream)
{
    return fgetc(stream);
}

int getchar(void)
{
    return fgetc(stdin);
}

int putc(int c, FILE *stream)
{
    return fputc(c, stream);
}

int putchar(int c)
{
    return fputc(c, stdout);
}

int puts(const char *s)
{
    if(EOF == fputs(s, stdout))
        return EOF;
    return fputc('\n', stdout);
}

int ungetc(int c, FILE *stream)
{
    if(__VALIDATE_STREAM(stream) < 0)
        return EOF;

    if(EOF == c)
        return EOF;

    stream->flags &= ~__NABLA_LIBC_FILE_FLAG_EOF;
    stream->pushback = c;
    return c;
}

size_t fread(void * restrict ptr, size_t size, size_t nmemb, FILE * restrict stream)
{
    size_t total = size * nmemb;
    size_t read = 0;

    if(__VALIDATE_STREAM(stream) < 0)
        return 0;

    if(total == 0)
        return 0;

    if(stream->flags & __NABLA_LIBC_FILE_FLAG_EOF)
        return 0;

    if(_IONBF != stream->buffer_mode)
    {
        if(stream->buffer_ptr != stream->buffer_end)
        {
            size_t count = ((size_t)(stream->buffer_end - stream->buffer_ptr) < total) ? 
                (size_t)(stream->buffer_end - stream->buffer_ptr) : total;

            memcpy(ptr, stream->buffer_ptr, count);
            read += count;
            stream->buffer_ptr += count;
        }

        size_t blocks = (total - read) / stream->buffer_size;
        if(0 != blocks)
        {
            stream->pos += (stream->buffer_end - stream->buffer_base);
            stream->buffer_ptr = stream->buffer_base;
            stream->buffer_end = stream->buffer_base;
            size_t count = __read_write((char*)ptr + read , blocks * stream->buffer_size, stream, __READ);
            if((size_t)(-1) == count)
                return read / size;

            stream->pos += count;
            read += count;
            if(count < (blocks * stream->buffer_size))
            {
                stream->flags |= __NABLA_LIBC_FILE_FLAG_EOF;
                return read / size;
            }
        }
        
        if(read != total)
        {
            stream->pos += (stream->buffer_end - stream->buffer_base);
            stream->buffer_ptr = stream->buffer_base;
            stream->buffer_end = stream->buffer_base;
            size_t count = __read_write(stream->buffer_base, stream->buffer_size, stream, __READ);
            if((size_t)(-1) == count)
                return read / size;
            
            stream->buffer_end = stream->buffer_base + count;
            if((read + count) < total)
            {
                memcpy((char*)ptr + read, stream->buffer_base, count);
                stream->flags |= __NABLA_LIBC_FILE_FLAG_EOF;
                stream->buffer_ptr += count;
                read += count;
            }
            else
            {
                memcpy((char*)ptr + read, stream->buffer_base, total - read);
                stream->buffer_ptr += (total - read);
                read = total;
            }
        }
    }
    else //no buffering
    {
        size_t count = __read_write(ptr, total, stream, __READ);
        if((size_t)(-1) == count)
        {
            return 0;
        }
        stream->pos += count;
        read += count;
    }
    
    return read / size;
}

size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb, FILE * restrict stream)
{
    size_t total = size * nmemb;
    size_t written = 0;
    const char *s = ptr;

    if(__VALIDATE_STREAM(stream) < 0)
        return 0;

    if(0 == total)
        return 0;

    if(_IOLBF == stream->buffer_mode)
    {
        while(1)
        {
            *(stream->buffer_ptr++) = s[written++];
            ++stream->buffer_end;
            if(('\n' == s[written - 1]) || (stream->buffer_end == (stream->buffer_base + stream->buffer_size)))
            {
                size_t count = __read_write(stream->buffer_base, stream->buffer_end - stream->buffer_base, stream, __WRITE);
                if((size_t)(-1) == count)
                {
                    stream->buffer_ptr = stream->buffer_base;
                    stream->buffer_end = stream->buffer_base;
                    return written / size;
                }
                stream->pos += count;
                if(count < (size_t)(stream->buffer_end - stream->buffer_base))
                {
                    stream->buffer_ptr = stream->buffer_base;
                    stream->buffer_end = stream->buffer_base;
                    return written / size;
                }
                stream->buffer_ptr = stream->buffer_base;
                stream->buffer_end = stream->buffer_base;
            }
            if(written == total)
                break;
        }
        return nmemb;
    }
    else if(_IOFBF == stream->buffer_mode)
    {
        //flush data from buffer if new data won't fit
        size_t current = stream->buffer_end - stream->buffer_base;
        if((total + current) >= stream->buffer_size)
        {
            size_t count = __read_write(stream->buffer_base, current, stream, __WRITE);
            if((size_t)(-1) == count)
                return 0;
            stream->pos += count;
            stream->buffer_end = stream->buffer_base;
            stream->buffer_ptr = stream->buffer_base;
            if(count < current)
                return 0;
        }
        //write data directly
        size_t blocks = total / stream->buffer_size;
        if(0 != blocks)
        {
            size_t count = __read_write((char*)ptr, blocks * stream->buffer_size, stream, __WRITE);
            if((size_t)(-1) == count)
                return written / size;

            stream->pos += count;
            written += count;
            if(count < (blocks * stream->buffer_size))
                return written / size;
        }
        //fill buffer with the remainder
        if(written != total)
        {
            memcpy(stream->buffer_end, (char*)ptr + written, total - written);
            stream->buffer_end += (total - written);
            stream->buffer_ptr = stream->buffer_end;
        }
        return nmemb;
    }
    else //no buffering
    {
        size_t count = __read_write((char*)s, total, stream, __WRITE);
        if((size_t)(-1) == count)
            return 0;
        if(count != total)
        {
            return count / size;
        }
        stream->pos += count;
        return nmemb;
    }
}