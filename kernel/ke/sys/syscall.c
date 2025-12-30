#include "syscall.h"
#include "io/fs/fs.h"
#include "ke/sched/sched.h"

#include "mmap.h"
#include "file.h"

#define SYSCALL_NONE 0 /**< No operation syscall */

static reg_t KeSyscallNone(reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5)
{
    UNUSED(arg1);
    UNUSED(arg2);
    UNUSED(arg3);
    UNUSED(arg4);
    UNUSED(arg5);
    return -OK;
}

static const KeSyscallHandler KeSyscallTable[] = 
{
    [SYSCALL_NONE] = KeSyscallNone,
    [SYSCALL_OPEN] = KeSyscallOpen,
    [SYSCALL_CLOSE] = KeSyscallClose,
    [SYSCALL_READ] = KeSyscallRead,
    [SYSCALL_WRITE] = KeSyscallWrite,
    [SYSCALL_MMAP] = KeSyscallMmap,
    [SYSCALL_EXMMAP] = KeSyscallExMmap,
    [SYSCALL_SET_TLS] = KeSyscallSetTls,
};

reg_t KePerformSyscall(reg_t code, reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5)
{
    if(
        (code >= (sizeof(KeSyscallTable) / sizeof(KeSyscallTable[0])))
        || (NULL == KeSyscallTable[code])
    )
        return -NOT_IMPLEMENTED;
    
    return KeSyscallTable[code](arg1, arg2, arg3, arg4, arg5);
}