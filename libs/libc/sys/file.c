#include "io/fs/fs.h"
#include "io/fs/vfs.h"
#include "helpers.h"
#include "sys/errno.h"
#include "sys/fcntl.h"
#include "sys/stat.h"
#include "stdio.h"

int _close(int fd)
{
    STATUS status = ApiCloseFile(fd);
    __NABLA_RETURN(status, 0);
}

int _open(const char *file, int flags, ...)
{
    //TODO: implement "mode" support
    int handle = -1;
    IoFileOpenMode fmode = 0;
    IoFileFlags fflags = 0;

    if(flags & O_RDWR)
        fmode |= IO_FILE_READ | IO_FILE_WRITE;
    else if(flags & O_RDONLY)
        fmode |= IO_FILE_READ;
    else if(flags & O_WRONLY)
        fmode |= IO_FILE_WRITE;
    if(!(flags & O_RDONLY) && (flags & O_APPEND))
        fmode |= IO_FILE_APPEND;
    if(flags & O_CREAT)
        fmode |= IO_FILE_CREATE;
    
    STATUS status = ApiOpenFile(file, fmode, fflags, &handle);
    __NABLA_RETURN(status, handle);
}

int _read(int file, char *ptr, int len)
{
    STATUS status = ApiReadFileSync(file, ptr, len, IO_OFFSET_CURRENT, nullptr);
    __NABLA_RETURN(status, 0);
}

int _write(int file, char *ptr, int len)
{
    STATUS status = ApiWriteFileSync(file, ptr, len, IO_OFFSET_CURRENT, nullptr);
    __NABLA_RETURN(status, 0);
}

int _lseek(int file, int ptr, int dir)
{
    IoSetFileOffsetMethod method;
    switch(dir)
    {
        case SEEK_SET:
            method = IO_SET_OFFSET;
            break;
        case SEEK_CUR:
            method = IO_SET_OFFSET_CUR;
            break;
        case SEEK_END:
            method = IO_SET_OFFSET_END;
            break;
        default:
            errno = EINVAL;
            return -1;
    }
    uint64_t current = 0;
    STATUS status = ApiSetFileOffset(file, ptr, method, &current);
    __NABLA_RETURN(status, (int)current);
}

int	_mkdir(const char *path, mode_t mode)
{
    //TODO: implement mkdir
    errno = ENOSYS;
    return -1;
}

static int __stat(int fd, const char *restrict path, struct stat *sbuf)
{
    struct IoFileAttributes attr;
    STATUS status = ApiGetFileAttributes(fd, nullptr, false, &attr);
    if(OK == status)
    {
        sbuf->st_dev = -1;
        sbuf->st_ino = -1;
        sbuf->st_nlink = attr.links;
        sbuf->st_uid = attr.uid;
        sbuf->st_gid = attr.gid;
        sbuf->st_rdev = -1;
        sbuf->st_size = attr.size;
        sbuf->st_blksize = attr.blockSize;
        sbuf->st_blocks = -1;
        sbuf->st_atim.tv_sec = attr.accessTime.seconds;
        sbuf->st_atim.tv_nsec = attr.accessTime.nanoseconds;
        sbuf->st_mtim.tv_sec = attr.changeTime.seconds;
        sbuf->st_mtim.tv_nsec = attr.changeTime.nanoseconds;
        sbuf->st_ctim.tv_sec = attr.statusChangeTime.seconds;
        sbuf->st_ctim.tv_nsec = attr.statusChangeTime.nanoseconds;
        switch(attr.type)
        {
            case IO_VFS_FILE:
                sbuf->st_mode = S_IFREG;
                break;
            case IO_VFS_DIRECTORY:
                sbuf->st_mode = S_IFDIR;
                break;
            case IO_VFS_LINK:
                sbuf->st_mode = S_IFLNK; //should not happen here
                break;
            default:
                sbuf->st_mode = S_IFMT; 
                //Use mask for different other file types. We don't have notion of character/block devices and Unix has no notion of a general device.
                break;
        }
    }
    __NABLA_RETURN(status, 0);
}

int	_fstat(int fd, struct stat *sbuf)
{
    return __stat(fd, nullptr, sbuf);
}

int	_stat(const char *__restrict path, struct stat *__restrict sbuf)
{
    return __stat(-1, path, sbuf);
}

int _isatty(int fd)
{
    struct IoFileAttributes attr;
    STATUS status = ApiGetFileAttributes(fd, nullptr, false, &attr);
    //assume that a character device is a TTY
    __NABLA_RETURN(status, !!(attr.auxFlags.characterDevice));
}

int _fcntl(int fd, int op, ...)
{
    errno = ENOSYS;
    return -1;
    //TODO: implement fcntl()
}

int _link(const char *__path1, const char *__path2)
{
    errno = ENOSYS;
    return -1;
    //TODO: implement link()
}

int _unlink(char *name)
{
    errno = ENOSYS;
    return -1;
    //TODO: implement unlink()
}

int _rename(const char *oldName, const char *newName)
{
    errno = ENOSYS;
    return -1;
    //TODO: implement rename()
}