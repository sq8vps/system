#include "file.h"
#include "io/fs/fs.h"

reg_t KeSyscallOpen(reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5)
{
    const char *file = (const char *)arg1;
    int mode = (int)arg2;
    int flags = (int)arg3;
    UNUSED(arg4);
    UNUSED(arg5);
    IoFileFlags ioFlags = 0;
    IoFileOpenMode ioMode = 0;
    int handle = -1;
    STATUS status = OK;

    if(flags & FILE_FLAG_DIRECT)
        ioFlags |= IO_FILE_FLAG_DIRECT;
    if(flags & FILE_FLAG_NO_WAIT)
        ioFlags |= IO_FILE_FLAG_NO_WAIT;
    if(flags & FILE_FLAG_SHARED)
        ioFlags |= IO_FILE_FLAG_SHARED;
    if(flags & FILE_NO_LINK_RESOLUTION)
        ioFlags |= IO_FILE_NO_LINK_RESOLUTION;

    if(mode & FILE_READ)
        ioMode = IO_FILE_READ;
    else if(mode & FILE_WRITE)
        ioMode = IO_FILE_WRITE;
    else if(mode & FILE_APPEND)
        ioMode = IO_FILE_APPEND;
    
    if(mode & FILE_CREATE)
        ioMode |= IO_FILE_CREATE;

    status = IoOpenFile(file, ioMode, ioFlags, &handle);
    if(OK == status)
        return (reg_t)handle;
    else
        return -status;
}

reg_t KeSyscallClose(reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5)
{
    int handle = (int)arg1;
    UNUSED(arg2);
    UNUSED(arg3);
    UNUSED(arg4);
    UNUSED(arg5);
    return -IoCloseFile(handle);
}

reg_t KeSyscallRead(reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5)
{
    int handle = (int)arg1;
    void *buffer = (void *)arg2;
    size_t size = (size_t)arg3;
    size_t offsetLo = (size_t)arg4;
    size_t offsetHi = (size_t)arg5;
    size_t actual = 0;
    STATUS status = OK;
#if SIZE_MAX >= UINT64_MAX
    uint64_t offset = offsetLo;
#else
    uint64_t offset = ((uint64_t)offsetHi << (sizeof(size_t) * 8)) | (uint64_t)offsetLo;
#endif
    status = IoReadFileSync(handle, buffer, size, offset, &actual);
    if(OK == status)
        return (reg_t)actual;
    else
        return -status;
}

reg_t KeSyscallWrite(reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5)
{
    int handle = (int)arg1;
    void *buffer = (void *)arg2;
    size_t size = (size_t)arg3;
    size_t offsetLo = (size_t)arg4;
    size_t offsetHi = (size_t)arg5;
    size_t actual = 0;
    STATUS status = OK;
#if SIZE_MAX >= UINT64_MAX
    uint64_t offset = offsetLo;
#else
    uint64_t offset = ((uint64_t)offsetHi << (sizeof(size_t) * 8)) | (uint64_t)offsetLo;
#endif
    status = IoWriteFileSync(handle, buffer, size, offset, &actual);
    if(OK == status)
        return (reg_t)actual;
    else
        return -status;
}