#include "gdt.h"
#include "msr.h"
#include "ke/sys/llsyscall.h"
#include "mm/tmem.h"

#include "ke/core/panic.h"

extern void I686Sysenter(void);

void I686InitializeSyscall(void)
{
    MsrSet(MSR_IA32_SYSENTER_CS, GDT_OFFSET(GDT_KERNEL_CS));
    MsrSet(MSR_IA32_SYSENTER_EIP, (uintptr_t)I686Sysenter);
}

STATUS I686ValidateSyscall(reg_t stack, reg_t code, size_t *argSize)
{
    if(!MmProbeUserMemory((const void*)stack, sizeof(void*), MM_TASK_MEMORY_READABLE | MM_TASK_MEMORY_WRITABLE))
    {
        //TODO: handle task termination
        KePanic(UNEXPECTED_FAULT);
    }

    const struct KeSyscallDescriptor *d = KeGetSyscallDescriptor(code);
    if(nullptr == d)
        return NOT_IMPLEMENTED;
    
    size_t total = 0;
    for(size_t i = 0; i < (sizeof(d->arg) / sizeof(d->arg[0])); i++)
    {
        if(0 == d->arg[i])
            break;
        total += ALIGN_UP(d->arg[i], sizeof(reg_t));
    }

    //By syscall convention, 5 registers (20 bytes) are used for arguments and the remaining ones must be on the stack
    //Also, the latest push is the return address
    if(total > 20)
    {
        if(!MmProbeUserMemory((const void*)(stack + sizeof(void*)), total - 20, MM_TASK_MEMORY_READABLE))
        {
            //TODO: handle task termination
            KePanic(UNEXPECTED_FAULT); 
        }
    }

    *argSize = total;
    return OK;
}