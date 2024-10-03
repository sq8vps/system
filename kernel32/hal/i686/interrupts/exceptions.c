#include "exceptions.h"
#include "ke/core/panic.h"
#include "hal/i686/interrupts/it.h"
#include "hal/i686/fpu.h"
#include "it/it.h"
#include "hal/i686/interrupts/it.h"
#include "hal/arch.h"
#include "hal/i686/memory.h"

//debug interrupt handlers

IT_HANDLER static void ItDebugHandler(struct ItFrame *f)
{
    
}

IT_HANDLER static void ItBreakpointHandler(struct ItFrame *f)
{

}

//FPU faults, easily recoverable

IT_HANDLER static void ItSimdFpuHandler(struct ItFrame *f)
{
    //TODO: implement SIMD handling
    while(1);
}

IT_HANDLER static void ItFpuHandler(struct ItFrame *f)
{
    FpuHandleException();
}

//faults possibly correctable in user mode and kernel mode

IT_HANDLER static void ItPageFaultHandler(struct ItFrame *f, uint32_t error)
{
    uintptr_t cr2;
    //obtain failing address from CR2 register
    ASM("mov %0,cr2" : "=r" (cr2) : );

    //handle lazy TLB shootdown
    MmMemoryFlags flags = I686GetPageFlagsFromPageFault(cr2);
    uint8_t ok = 0;
    ok |= (flags & MM_FLAG_PRESENT) && !(error & 1); //P-flag is 0 - page not present in TLB
    ok |= (flags & MM_FLAG_WRITABLE) && (error & 2); //W/R-flag is 1 - write was attempted, but page is read-only in TLB
    ok |= (flags & MM_FLAG_USER_MODE) && (error & 4); //U/S-flag is 1 - access was in user mode, but page is kernel-only in TLB

    if(ok)
        I686_INVALIDATE_TLB(cr2);
    else
        KePanicIPEx(f->ip, KERNEL_MODE_FAULT, PAGE_FAULT, cr2, error, 0);
}

//faults correctable in user mode

IT_HANDLER static void ItAlignmentCheckHandler(struct ItFrame *f, uint32_t error)
{
    //should never be called in kernel mode
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, UNEXPECTED_INTEL_TRAP, error, 0, 0);
}

IT_HANDLER static void ItGeneralProtectionHandler(struct ItFrame *f, uint32_t error)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, GENERAL_PROTECTION_FAULT, error, 0, 0);
}

IT_HANDLER static void ItDivisionByZeroHandler(struct ItFrame *f)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, DIVIDE_ERROR, 0, 0, 0);
}

IT_HANDLER static void ItBoundExceededHandler(struct ItFrame *f)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, BOUND_RANGE_EXCEEDED, 0, 0, 0);
}

IT_HANDLER static void ItInvalidOpcodeHandler(struct ItFrame *f)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, INVALID_OPCODE, 0, 0, 0);
}

IT_HANDLER static void ItOverflowHandler(struct ItFrame *f)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, OVERFLOW, 0, 0, 0);
}

//fatal errors

IT_HANDLER static void ItNmiHandler(struct ItFrame *f)
{
    KePanicEx(KERNEL_MODE_FAULT, NON_MASKABLE_INTERRUPT, 0, 0, 0);
}

IT_HANDLER static void ItDoubleFaultHandler(struct ItFrame *f, uint32_t error)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, DOUBLE_FAULT, error, 0, 0);
}

IT_HANDLER static void ItMachineCheckHandler(struct ItFrame *f)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, MACHINE_CHECK, 0, 0, 0);
}

IT_HANDLER static void ItDeviceUnavailableHandler(struct ItFrame *f)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, FPU_NOT_AVAILABLE, 0, 0, 0);
}

IT_HANDLER static void ItCoprocessorOverrunHandler(struct ItFrame *f)
{
    //should not be called at all
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, COPROCESSOR_SEGMENT_OVERRUN, 0, 0, 0);
}

IT_HANDLER static void ItInvalidTssHandler(struct ItFrame *f, uint32_t error)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, INVALID_TSS, error, 0, 0);
}

IT_HANDLER static void ItSegmentNotPresentHandler(struct ItFrame *f, uint32_t error)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, SEGMENT_NOT_PRESENT, error, 0, 0);
}

IT_HANDLER static void ItStackFaultHandler(struct ItFrame *f, uint32_t error)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, STACK_FAULT, error, 0, 0);
}

//dont-know-what-to-do-with-them-for-now faults

IT_HANDLER static void ItVirtualizationExceptionHandler(struct ItFrame *f)
{
    KePanicIPEx(f->ip, KERNEL_MODE_FAULT, VIRTUALIZATION_EXCEPTION, 0, 0, 0);
}

IT_HANDLER static void ItControlPriotectionHandler(struct ItFrame *f, uint32_t error)
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

void I686InstallAllExceptionHandlers(uint16_t cpu)
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

