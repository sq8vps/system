#include "mmap.h"
#include "mm/tmem.h"
#include "mm/mm.h"
#include "ke/task/task.h"
#include "hal/task.h"
#include "ke/sched/sched.h"

static reg_t KeDoSyscallMmap(void *addr, size_t length, int flags, int fd, size_t alignment, uint64_t offset, size_t limit)
{
    STATUS status = OK;
    if((NULL != addr) && !IS_USER_MEMORY((uintptr_t)addr, length))
        return -BAD_PARAMETER;

    if((flags & MMAP_FILE) && (fd < 0))
        return -BAD_PARAMETER;

    if(!(flags & MMAP_FILE))
        fd = -1;

    enum MmTaskMemoryFlags mmFlags = 0;
    if(flags & MMAP_READABLE)
        mmFlags |= MM_TASK_MEMORY_READABLE;
    if(flags & MMAP_WRITABLE)
        mmFlags |= MM_TASK_MEMORY_WRITABLE;
    if(flags & MMAP_EXECUTABLE)
        mmFlags |= MM_TASK_MEMORY_EXECUTABLE;
    if(flags & MMAP_FILE)
        mmFlags |= MM_TASK_MEMORY_WRITE_THROUGH;
    if(flags & MMAP_REVERSED)
        mmFlags |= MM_TASK_MEMORY_REVERSED;
    if(flags & MMAP_GROWABLE)
        mmFlags |= MM_TASK_MEMORY_GROWABLE;
    if(flags & MMAP_FIXED)
        mmFlags |= MM_TASK_MEMORY_FIXED;

    status = MmMapTaskMemory(addr, length, mmFlags, fd, alignment, offset, limit, &addr);
    if(OK == status)
        return (reg_t)addr;
    else
        return -status;  
}

reg_t KeSyscallMmap(reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5)
{
    void *addr = (void*)arg1;
    size_t length = (size_t)arg2;
    int flags = (int)arg3;
    int fd = (int)arg4;
    size_t offset = (size_t)arg5;

    return KeDoSyscallMmap(addr, length, flags, fd, 0, offset, 0);
}

reg_t KeSyscallExMmap(reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5)
{
    void *addr = (void*)arg1;
    size_t length = (size_t)arg2;
    int flags = (int)arg3;
    int fd = (int)arg4;
    const struct exmmap_params *const params = (const struct exmmap_params *const)arg5;

    size_t alignment = 0;
    uint64_t offset = 0;
    size_t limit = 0;

    if(NULL != params)
    {
        if(!IS_USER_MEMORY((uintptr_t)params, sizeof(*params)))
            return -BAD_PARAMETER;
        struct MmTaskMemory *m = MmGetTaskMemoryDescriptor(params);
        if(NULL == m)
            return -BAD_PARAMETER;
        
        if(((uintptr_t)params < (uintptr_t)m->base) || 
           (((uintptr_t)params + sizeof(*params)) >= (uintptr_t)m->end))
            return -BAD_PARAMETER;

        offset = params->offset;
        limit = params->limit;
        alignment = params->alignment;
    }

    return KeDoSyscallMmap(addr, length, flags, fd, alignment, offset, limit);
}

reg_t KeSyscallMunmap(reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5)
{
    const void *const *addr = (const void *const)arg1;
    size_t length = (size_t)arg2;
    UNUSED(arg3);
    UNUSED(arg4);
    UNUSED(arg5);
    return MmUnmapTaskMemory(addr, length);
}

reg_t KeSyscallSetTls(reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5)
{
    void *tls = (void*)arg1;
    UNUSED(arg2);
    UNUSED(arg3);
    UNUSED(arg4);
    UNUSED(arg5);
    KeSetThreadLocalStorage(KeGetCurrentTask(), tls);
    HalUpdateTls(tls);
    return OK;
}