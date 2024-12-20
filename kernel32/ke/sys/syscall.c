#include "syscall.h"
#include "io/fs/fs.h"
#include "ke/sched/sched.h"

enum KeSyscallCode
{
    SYSCALL_NONE = 0,
    SYSCALL_EXIT = 1,
    SYSCALL_OPEN = 2,
    SYSCALL_CLOSE = 3,
    SYSCALL_READ = 4,
    SYSCALL_WRITE = 5,
    _SYSCALL_LIMIT,
};

typedef STATUS (*KeSyscallHandler)(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5);

static STATUS KeSyscallNone(void)
{
    return OK;
}

static int KeSyscallOpen(const char *file, IoFileOpenMode mode, IoFileFlags flags)
{
    int handle = -1;
    IoOpenFile(file, mode, flags, &handle);
    return handle;
}

static STATUS KeSyscallClose(int handle)
{
    return IoCloseFile(handle);
}

static size_t KeSyscallRead(int handle, void *buffer, size_t size, uint64_t offset)
{
    size_t actualSize = 0;
    IoReadFileSync(handle, buffer, size, offset, &actualSize);
    return actualSize;
}

static size_t KeSyscallWrite(int handle, void *buffer, size_t size, uint64_t offset)
{
    size_t actualSize = 0;
    IoWriteFileSync(handle, buffer, size, offset, &actualSize);
    return actualSize;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
static KeSyscallHandler KeSyscallTable[] = 
{
    [SYSCALL_NONE] = (KeSyscallHandler)KeSyscallNone,
    [SYSCALL_OPEN] = (KeSyscallHandler)KeSyscallOpen,
    [SYSCALL_CLOSE] = (KeSyscallHandler)KeSyscallClose,
    [SYSCALL_READ] = (KeSyscallHandler)KeSyscallRead,
    [SYSCALL_WRITE] = (KeSyscallHandler)KeSyscallWrite,
};
#pragma GCC diagnostic pop

STATUS KePerformSyscall(uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{
    if(code >= _SYSCALL_LIMIT)
        return SYSCALL_CODE_UNKNOWN;
    
    return KeSyscallTable[code](arg1, arg2, arg3, arg4, arg5);
}