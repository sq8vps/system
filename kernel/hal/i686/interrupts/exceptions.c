#include "exceptions.h"
#include "ke/core/panic.h"
#include "hal/i686/interrupts/it.h"
#include "hal/i686/fpu.h"
#include "it/it.h"
#include "hal/i686/interrupts/it.h"
#include "hal/arch.h"
#include "hal/i686/memory.h"

//debug interrupt handlers

ISR static void ItDebugHandler(struct ItFrame *f)
{
    UNUSED(f);
}

ISR static void ItBreakpointHandler(struct ItFrame *f)
{
    UNUSED(f);
}

//FPU faults, easily recoverable

ISR static void ItSimdFpuHandler(struct ItFrame *f)
{
    UNUSED(f);
    //TODO: implement SIMD handling
    while(1);
}

ISR static void ItFpuHandler(struct ItFrame *f)
{
    UNUSED(f);
    FpuHandleException();
}

//faults possibly correctable in user mode and kernel mode

ISR static void ItPageFaultHandler(struct ItFrame *f, uint32_t error)
{
    UNUSED(f);
    reg_t cr2;
    //obtain failing address from CR2 register
    ASM("mov %0,cr2" : "=r" (cr2) : );

    //handle lazy TLB shootdown
    MmMemoryFlags flags = I686GetPageFlagsFromPageFault(cr2);
    uint8_t ok = 0;
    ok |= (flags & MM_FLAG_PRESENT) && !(error & 1); //P-flag is 0 - page not present in TLB
    ok |= (flags & MM_FLAG_WRITABLE) && (error & 2); //W/R-flag is 1 - write was attempted, but page is read-only in TLB
    ok |= (flags & MM_FLAG_USER_MODE) && (error & 4); //U/S-flag is 1 - access was in user mode, but page is kernel-only in TLB
    if(!(flags & MM_FLAG_USER_MODE) && (error & 4))
        ok = false;

    if(ok)
        I686_INVALIDATE_TLB(cr2);
    else
    {
        reg_t arg1 = 0, arg2 = 0;
        if(!(error & 1)) //P-flag not set
            arg1 = 0x1;
        else if(error & 0x20) //PK-flag
            arg1 = 0x8;
        else if(error & 0x8) //R-flag
            arg1 = 0x9;
        else if(0x2 == (error & 0x10FA)) //exclude P and U/S, if W/R is set, then page is read only
            arg1 = 0x2;
        else if(0x10 == (error & 0x10FA)) //exclude P and U/S, if I/D is set, then page is no-execute
            arg1 = 0x3;
        else
            arg1 = 0x1000;

        if(error & 2) //W/R-flag
            arg2 = 0x1;
        else if(error & 0x10) //I/D-flag
            arg2 = 0x2;
        
        KePanicIPEx(f->ip, PAGE_FAULT, cr2, arg1, arg2, (reg_t)flags);
    }
}

//faults correctable in user mode

ISR static void ItAlignmentCheckHandler(struct ItFrame *f, uint32_t error)
{
    //should never be called in kernel mode
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, UNEXPECTED_INTEL_TRAP, error, 0, 0);
}

ISR static void ItGeneralProtectionHandler(struct ItFrame *f, uint32_t error)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, GENERAL_PROTECTION_FAULT, error, 0, 0);
}

ISR static void ItDivisionByZeroHandler(struct ItFrame *f)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, DIVIDE_ERROR, 0, 0, 0);
}

ISR static void ItBoundExceededHandler(struct ItFrame *f)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, BOUND_RANGE_EXCEEDED, 0, 0, 0);
}

ISR static void ItInvalidOpcodeHandler(struct ItFrame *f)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, INVALID_OPCODE, 0, 0, 0);
}

ISR static void ItOverflowHandler(struct ItFrame *f)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, OVERFLOW, 0, 0, 0);
}

//fatal errors

ISR static void ItNmiHandler(struct ItFrame *f)
{
    UNUSED(f);
    KePanicEx(KERNEL_MODE_FAULT, NON_MASKABLE_INTERRUPT, 0, 0, 0);
}

ISR static void ItDoubleFaultHandler(struct ItFrame *f, uint32_t error)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, DOUBLE_FAULT, error, 0, 0);
}

ISR static void ItMachineCheckHandler(struct ItFrame *f)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, MACHINE_CHECK, 0, 0, 0);
}

ISR static void ItDeviceUnavailableHandler(struct ItFrame *f)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, FPU_NOT_AVAILABLE, 0, 0, 0);
}

ISR static void ItCoprocessorOverrunHandler(struct ItFrame *f)
{
    //should not be called at all
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, COPROCESSOR_SEGMENT_OVERRUN, 0, 0, 0);
}

ISR static void ItInvalidTssHandler(struct ItFrame *f, uint32_t error)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, INVALID_TSS, error, 0, 0);
}

ISR static void ItSegmentNotPresentHandler(struct ItFrame *f, uint32_t error)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, SEGMENT_NOT_PRESENT, error, 0, 0);
}

ISR static void ItStackFaultHandler(struct ItFrame *f, uint32_t error)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, STACK_FAULT, error, 0, 0);
}

//dont-know-what-to-do-with-them-for-now faults

ISR static void ItVirtualizationExceptionHandler(struct ItFrame *f)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, VIRTUALIZATION_EXCEPTION, 0, 0, 0);
}

ISR static void ItControlPriotectionHandler(struct ItFrame *f, uint32_t error)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, CONTROL_PROTECTION_EXCEPTION, error, 0, 0);
}

/**
 * @brief Enum containing exception vectors
*/
enum ItExceptionVector
{
    IT_EXCEPTION_DIVIDE = 0,
    IT_EXCEPTION_DEBUG = 1,
    IT_EXCEPTION_NMI = 2,
    IT_EXCEPTION_BREAKPOINT = 3,
    IT_EXCEPTION_OVERFLOW = 4,
    IT_EXCEPTION_BOUND_EXCEEDED = 5,
    IT_EXCEPTION_INVALID_OPCODE = 6,
    IT_EXCEPTION_DEVICE_UNAVAILABLE = 7,
    IT_EXCEPTION_DOUBLE_FAULT = 8,
    IT_EXCEPTION_COPROCESSOR_OVERRUN = 9,
    IT_EXCEPTION_INVALID_TSS = 10,
    IT_EXCEPTION_SEGMENT_NOT_PRESENT = 11,
    IT_EXCEPTION_STACK_FAULT = 12,
    IT_EXCEPTION_GENERAL_PROTECTION = 13,
    IT_EXCEPTION_PAGE_FAULT = 14,
    IT_EXCEPTION_FPU_ERROR = 16,
    IT_EXCEPTION_ALIGNMENT_CHECK = 17,
    IT_EXCEPTION_MACHINE_CHECK = 18,
    IT_EXCEPTION_SIMD_FPU = 19,
    IT_EXCEPTION_VIRTUALIZATION = 20,
    IT_EXCEPTION_CONTROL_PROTECTION = 21,
};

void I686InstallAllExceptionHandlers(uint32_t cpu)
{
    I686InstallExceptionHandler(cpu, IT_EXCEPTION_DIVIDE, ItDivisionByZeroHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_DEBUG, ItDebugHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_NMI, ItNmiHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_BREAKPOINT, ItBreakpointHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_OVERFLOW, ItOverflowHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_BOUND_EXCEEDED, ItBoundExceededHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_INVALID_OPCODE, ItInvalidOpcodeHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_DEVICE_UNAVAILABLE, ItDeviceUnavailableHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_DOUBLE_FAULT, ItDoubleFaultHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_COPROCESSOR_OVERRUN, ItCoprocessorOverrunHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_INVALID_TSS, ItInvalidTssHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_SEGMENT_NOT_PRESENT, ItSegmentNotPresentHandler); 
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_STACK_FAULT, ItStackFaultHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_GENERAL_PROTECTION, ItGeneralProtectionHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_PAGE_FAULT, ItPageFaultHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_FPU_ERROR, ItFpuHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_ALIGNMENT_CHECK, ItAlignmentCheckHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_MACHINE_CHECK, ItMachineCheckHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_SIMD_FPU, ItSimdFpuHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_VIRTUALIZATION, ItVirtualizationExceptionHandler);
	I686InstallExceptionHandler(cpu, IT_EXCEPTION_CONTROL_PROTECTION, ItControlPriotectionHandler);
}

