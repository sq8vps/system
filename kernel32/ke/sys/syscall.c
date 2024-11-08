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
    IoOpenFile(file, mode, flags, KeGetCurrentTask(), &handle);
    return handle;
}

static STATUS KeSyscallClose(int handle)
{
    return IoCloseFile(KeGetCurrentTask(), handle);
}

static size_t KeSyscallRead(int handle, void *buffer, size_t size, uint64_t offset)
{
    size_t actualSize = 0;
    IoReadFileSync(KeGetCurrentTask(), handle, buffer, size, offset, &actualSize);
    return actualSize;
}

static size_t KeSyscallWrite(int handle, void *buffer, size_t size, uint64_t offset)
{
    size_t actualSize = 0;
    IoWriteFileSync(KeGetCurrentTask(), handle, buffer, size, offset, &actualSize);
    return actualSize;
}

static KeSyscallHandler KeSyscallTable[] = 
{
    [SYSCALL_NONE] = (KeSyscallHandler)KeSyscallNone,
    [SYSCALL_OPEN] = (KeSyscallHandler)KeSyscallOpen,
    [SYSCALL_CLOSE] = (KeSyscallHandler)KeSyscallClose,
    [SYSCALL_READ] = (KeSyscallHandler)KeSyscallRead,
    [SYSCALL_WRITE] = (KeSyscallHandler)KeSyscallWrite,
};

STATUS KePerformSyscall(uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{
    if(code >= _SYSCALL_LIMIT)
        return SYSCALL_CODE_UNKNOWN;
    
    return KeSyscallTable[code](arg1, arg2, arg3, arg4, arg5);
}