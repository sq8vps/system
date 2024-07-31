#include "exceptions.h"
#include "ke/core/panic.h"
#include "it/it.h"
#include "hal/i686/fpu.h"

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

extern STATUS InstallExceptionHandler(enum ItExceptionVector vector, void *isr);

void InstallExceptionHandlers(void)
{
    InstallExceptionHandler(IT_EXCEPTION_DIVIDE, ItDivisionByZeroHandler);
	InstallExceptionHandler(IT_EXCEPTION_DEBUG, ItDebugHandler);
	InstallExceptionHandler(IT_EXCEPTION_NMI, ItNmiHandler);
	InstallExceptionHandler(IT_EXCEPTION_BREAKPOINT, ItBreakpointHandler);
	InstallExceptionHandler(IT_EXCEPTION_OVERFLOW, ItOverflowHandler);
	InstallExceptionHandler(IT_EXCEPTION_BOUND_EXCEEDED, ItBoundExceededHandler);
	InstallExceptionHandler(IT_EXCEPTION_INVALID_OPCODE, ItInvalidOpcodeHandler);
	InstallExceptionHandler(IT_EXCEPTION_DEVICE_UNAVAILABLE, ItDeviceUnavailableHandler);
	InstallExceptionHandler(IT_EXCEPTION_DOUBLE_FAULT, ItDoubleFaultHandler);
	InstallExceptionHandler(IT_EXCEPTION_COPROCESSOR_OVERRUN, ItCoprocessorOverrunHandler);
	InstallExceptionHandler(IT_EXCEPTION_INVALID_TSS, ItInvalidTssHandler);
	InstallExceptionHandler(IT_EXCEPTION_SEGMENT_NOT_PRESENT, ItSegmentNotPresentHandler); 
	InstallExceptionHandler(IT_EXCEPTION_STACK_FAULT, ItStackFaultHandler);
	InstallExceptionHandler(IT_EXCEPTION_GENERAL_PROTECTION, ItGeneralProtectionHandler);
	InstallExceptionHandler(IT_EXCEPTION_PAGE_FAULT, ItPageFaultHandler);
	InstallExceptionHandler(IT_EXCEPTION_FPU_ERROR, ItFpuHandler);
	InstallExceptionHandler(IT_EXCEPTION_ALIGNMENT_CHECK, ItAlignmentCheckHandler);
	InstallExceptionHandler(IT_EXCEPTION_MACHINE_CHECK, ItMachineCheckHandler);
	InstallExceptionHandler(IT_EXCEPTION_SIMD_FPU, ItSimdFpuHandler);
	InstallExceptionHandler(IT_EXCEPTION_VIRTUALIZATION, ItVirtualizationExceptionHandler);
	InstallExceptionHandler(IT_EXCEPTION_CONTROL_PROTECTION, ItControlPriotectionHandler);
}

