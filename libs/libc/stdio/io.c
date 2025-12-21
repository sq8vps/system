#include "stdio.h"
#include "nabla/nabla.h"
#include "sys/file.h"
#include "errno.h"
#include "nabla/defs.h"
#include "stdlib.h"
#include "string.h"

static size_t __read_write(void * restrict ptr, size_t size, FILE * restrict stream, bool _write)
{
    if(0 == size)
        return 0;

    if(NULL == ptr)
    {
        *__errno() = -EINVAL;
        return -1;
    }
    
    if(__VALIDATE_STREAM(stream) < 0)
        return -1; 

    if(__WRITE == _write)
    {
        return write(stream->handle, ptr, size, 
            (sizeof(stream->pos) == sizeof(size_t)) ? stream->pos : (stream->pos & ((1ULL << (8 * sizeof(size_t)))) - 1ULL),
            (sizeof(stream->pos) == sizeof(size_t)) ? 0 : (stream->pos >> (8 * sizeof(size_t))));
    }
    else
    {
        return read(stream->handle, ptr, size, 
            (sizeof(stream->pos) == sizeof(size_t)) ? stream->pos : (stream->pos & ((1ULL << (8 * sizeof(size_t)))) - 1ULL),
            (sizeof(stream->pos) == sizeof(size_t)) ? 0 : (stream->pos >> (8 * sizeof(size_t))));
    }
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
                    stream->buffer_ptr += written;
                    return s;
                }
            }
            stream->buffer_ptr += count;
            stream->pos += count;
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

                if(count > (size_t)(n - written))
                    count = (size_t)(n - written);

                for(int i = 0; i < count; i++)
                {
                    s[written++] = *(stream->buffer_ptr++);
                    if(('\n' == s[written - 1]) || (written == (n - 1)))
                    {
                        s[written] = '\0';
                        stream->buffer_ptr += written;
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
        {
            *__errno() = -ENOMEM;
            return NULL;
        }

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
    {
        *__errno() = -EINVAL;
        return EOF;
    }

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
                || (stream->buffer_end < (stream->buffer_base + stream->buffer_size)))
            {
                size_t count = __read_write(stream->buffer_base, stream->buffer_end - stream->buffer_base, stream, __WRITE);
                if(count != (stream->buffer_end - stream->buffer_base))
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
        size_t count = __read_write(s, len, stream, __WRITE);
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
        return NULL;

    if(total == 0)
        return 0;

    if(stream->flags & __NABLA_LIBC_FILE_FLAG_EOF)
        return 0;

    if(_IONBF != stream->buffer_mode)
    {
        if(stream->buffer_ptr != stream->buffer_end)
        {
            size_t count = ((stream->buffer_end - stream->buffer_ptr) < total) ? 
                (stream->buffer_end - stream->buffer_ptr) : total;

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
            if((('\n' == s[written - 1]) && (_IOLBF == stream->buffer_mode)) 
                || (stream->buffer_end < (stream->buffer_base + stream->buffer_size)))
            {
                size_t count = __read_write(stream->buffer_base, stream->buffer_end - stream->buffer_base, stream, __WRITE);
                if((size_t)(-1) == count)
                {
                    stream->buffer_ptr = stream->buffer_base;
                    stream->buffer_end = stream->buffer_base;
                    return written / size;
                }
                stream->pos += count;
                if(count < (stream->buffer_end - stream->buffer_base))
                {
                    stream->buffer_ptr = stream->buffer_base;
                    stream->buffer_end = stream->buffer_base;
                    return written / size;
                }
                stream->buffer_ptr = stream->buffer_base;
                stream->buffer_end = stream->buffer_base;
            }
        }
    }
    else if(_IOFBF == stream->buffer_mode)
    {
        if(stream->buffer_ptr != stream->buffer_end)
        {
            size_t count = ((stream->buffer_end - stream->buffer_ptr) < total) ? 
                (stream->buffer_end - stream->buffer_ptr) : total;

            __read_write(stream->buffer_ptr)
            
            memcpy(ptr, stream->buffer_ptr, count);
            read += count;
            stream->buffer_ptr += count;
        }

        size_t blocks = total / stream->buffer_size;
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
    }
    else //no buffering
    {
        size_t count = __read_write(s, total, stream, __WRITE);
        if(count != total)
        {
            return EOF;
        }
        stream->pos += count;
        return 0;
    }
}