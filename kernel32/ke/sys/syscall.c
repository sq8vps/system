#include "syscall.h"

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

static KeSyscallHandler KeSyscallTable[] = 
{
    [SYSCALL_NONE] = (KeSyscallHandler)KeSyscallNone,
};

STATUS KePerformSyscall(uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
{
    if(code >= _SYSCALL_LIMIT)
        return SYSCALL_CODE_UNKNOWN;
    
    return KeSyscallTable[code](arg1, arg2, arg3, arg4, arg5);
}