#ifndef NABLA_LIBC_STDIO_H
#define NABLA_LIBC_STDIO_H

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

#define __STDC_VERSION_STDIO_H__ 202311L

typedef uint64_t fpos_t;

typedef struct
{
    unsigned long int flags; /**< File magic and flags */
    int handle; /**< Kernel file handle number */
    fpos_t pos; /**< Current file position (aligned to the base of the buffer) */
    char *buffer_base; /**< Buffer base (constant) */
    char *buffer_ptr; /**< Buffer pointer (movable) */
    char *buffer_end; /**< Buffer end (of available data) */
    size_t buffer_size; /**< Maximum size of the buffer */
    int buffer_mode; /**< Buffering mode: _IOFBF, _IOLBF, or _IONBF */
    int pushback; /**< Pushback character for ungetc() */
} FILE;

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define BUFSIZ 4096

#define EOF (-1)

#define FOPEN_MAX 64
#define FILENAME_MAX 4096

#define _PRINTF_NAN_LEN_MAX 3

#define L_tmpnam 64

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define TMP_MAX 32

#define _ATTRIBUTE_PRINTF_LIKE(fmt, ellipsis) __attribute__((format(printf, fmt, ellipsis)))

extern FILE *stderr;
extern FILE *stdin;
extern FILE *stdout;

#define __STDIN_HANDLE 0
#define __STDOUT_HANDLE 1
#define __STDERR_HANDLE 2

int remove(const char *filename);
int rename(const char *old_filename, const char *new_filename);
FILE *tmpfile(void);
char *tmpnam(char *str);

int fclose(FILE *stream);
int fflush(FILE *stream);
FILE *fopen(const char * restrict filename, const char * restrict mode);
FILE *freopen(const char * restrict filename, const char * restrict mode, FILE * restrict stream);
void setbuf(FILE * restrict stream, char * restrict buf);
int setvbuf(FILE * restrict stream, char * restrict buf, int mode, size_t size);

int fprintf(FILE * restrict stream, const char * restrict format, ...) _ATTRIBUTE_PRINTF_LIKE(2, 3);
int fscanf(FILE * restrict stream, const char * restrict format, ...) _ATTRIBUTE_PRINTF_LIKE(2, 3);
int printf(const char * restrict format, ...) _ATTRIBUTE_PRINTF_LIKE(1, 2);
int scanf(const char * restrict format, ...) _ATTRIBUTE_PRINTF_LIKE(1, 2);
int snprintf(char * restrict str, size_t size, const char * restrict format, ...) _ATTRIBUTE_PRINTF_LIKE(3, 4);
int sprintf(char * restrict str, const char * restrict format, ...) _ATTRIBUTE_PRINTF_LIKE(2, 3);
int sscanf(const char * restrict str, const char * restrict format, ...) _ATTRIBUTE_PRINTF_LIKE(2, 3);
int vfprintf(FILE * restrict stream, const char * restrict format, va_list args);
int vfscanf(FILE * restrict stream, const char * restrict format, va_list arg);
int vprintf(const char * restrict format, va_list arg);
int vscanf(const char * restrict format, va_list arg);
int vsnprintf(char * restrict s, size_t n, const char * restrict format, va_list arg);
int vsprintf(char * restrict s, const char * restrict format, va_list arg);
int vsscanf(const char * restrict s, const char * restrict format, va_list arg);

int fgetc(FILE *stream);
char *fgets(char * restrict s, int n, FILE * restrict stream);
int fputc(int c, FILE *stream);
int fputs(const char * restrict s, FILE * restrict stream);
int getc(FILE *stream);
int getchar(void);
int putc(int c, FILE *stream);
int putchar(int c);
int puts(const char *s);
int ungetc(int c, FILE *stream);

size_t fread(void * restrict ptr, size_t size, size_t nmemb, FILE * restrict stream);
size_t fwrite(const void * restrict ptr, size_t size, size_t nmemb, FILE * restrict stream);

int fgetpos(FILE * restrict stream, fpos_t * restrict pos);
int fseek(FILE *stream, long int offset, int whence);
int fsetpos(FILE *stream, const fpos_t *pos);
long int ftell(FILE *stream);
void rewind(FILE *stream);

void clearerr(FILE *stream);
int feof(FILE *stream);
int ferror(FILE *stream);
void perror(const char *s);

#endif