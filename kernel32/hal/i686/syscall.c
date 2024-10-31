#include "gdt.h"
#include "msr.h"
#include "ke/core/panic.h"
#include "hal/mm.h"
#include "ke/sys/syscall.h"

extern void I686Sysenter(void);

void I686InitializeSyscall(void)
{
    MsrSet(MSR_IA32_SYSENTER_CS, GDT_OFFSET(GDT_KERNEL_CS));
    MsrSet(MSR_IA32_SYSENTER_EIP, (uintptr_t)I686Sysenter);
}

void I686VerifySyscall(uintptr_t ret, uintptr_t stack)
{
    STATUS status = OK;
    MmMemoryFlags mflags = 0;

    status = HalGetPageFlags(ret, &mflags);
    if((OK != status) || (mflags & MM_FLAG_NON_EXECUTABLE) || !(mflags & MM_FLAG_USER_MODE) || !(mflags & MM_FLAG_PRESENT))
    {
        //return address invalid, terminate task
        KePanic(UNEXPECTED_FAULT);
    }
    status = HalGetPageFlags(stack, &mflags);
    if((OK != status) || !(mflags & MM_FLAG_USER_MODE) || !(mflags & MM_FLAG_PRESENT) || !(mflags & MM_FLAG_WRITABLE) || (mflags & MM_FLAG_READ_ONLY))
    {
        //stack address invalid, terminate task
        KePanic(UNEXPECTED_FAULT);
    }
}