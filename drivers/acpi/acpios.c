#define DISABLE_KERNEL_STDLIB

#include "kernel.h"
#include "acpica/include/acpi.h"
#include "logging.h"


ACPI_STATUS AcpiOsInitialize(void)
{
    return AE_OK;
}

ACPI_STATUS AcpiOsTerminate(void)
{
    return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void)
{
    ACPI_PHYSICAL_ADDRESS a = 0;
    AcpiFindRootPointer(&a);
    return a;
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject, ACPI_STRING *NewValue)
{
    *NewValue = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable)
{
    *NewTable = NULL;
    return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewTableLength)
{
    *NewAddress = 0;
    *NewTableLength = 0;
    return AE_OK;
}

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length)
{
    return MmMapDynamicMemory(PhysicalAddress, Length, MM_FLAG_WRITABLE);
}

void AcpiOsUnmapMemory(void *where, ACPI_SIZE length)
{
    MmUnmapDynamicMemory(where);
}

ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress, ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{
    uintptr_t p = 0;
    STATUS status = HalGetPhysicalAddress((uintptr_t)LogicalAddress, &p);
    *PhysicalAddress = p;
    if(OK != status)
        return AE_ERROR;
    
    return AE_OK;
}

void *AcpiOsAllocate(ACPI_SIZE Size)
{
    return MmAllocateKernelHeap(Size);
}

void AcpiOsFree(void *Memory)
{
    MmFreeKernelHeap(Memory);
}

BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length)
{
    ACPI_SIZE k = 0;
    while(k < Length)
    {
        MmMemoryFlags flags;
        if(OK != HalGetPageFlags((uintptr_t)Memory + k, &flags))
            return false;
        
        if((flags & MM_FLAG_PRESENT) == 0)
            return false;
        
        k += MM_PAGE_SIZE;
    }
    return true;
}

BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length)
{
    ACPI_SIZE k = 0;
    while(k < Length)
    {
        MmMemoryFlags flags;
        if(OK != HalGetPageFlags((uintptr_t)Memory + k, &flags))
            return false;
        
        if(((flags & MM_FLAG_PRESENT) == 0) || ((flags & MM_FLAG_WRITABLE) == 0))
            return false;
        
        k += MM_PAGE_SIZE;
    }
    return true;
}

ACPI_THREAD_ID AcpiOsGetThreadId(void)
{
    return KeGetCurrentTask()->tid;
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context)
{
    struct KeTaskControlBlock *tcb;
    if(OK != KeCreateProcessRaw("ACPI worker", NULL, PL_KERNEL, Function, Context, &tcb))
        return AE_ERROR;
    
    KeEnableTask(tcb);
    return AE_OK;
}

void AcpiOsSleep(UINT64 Milliseconds)
{
    KeSleep(MS_TO_NS(Milliseconds));
}

void AcpiOsStall(UINT32 Microseconds)
{
    KeDelay(US_TO_NS(Microseconds));
}

void AcpiOsWaitEventsComplete(void)
{

}

ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX *OutHandle)
{
    *OutHandle = MmAllocateKernelHeap(sizeof(KeMutex));
    if(NULL == *OutHandle)
        return AE_NO_MEMORY;
    
    (*OutHandle)->lock = 0;
    (*OutHandle)->queueBottom = NULL;
    (*OutHandle)->queueTop = NULL;
    (*OutHandle)->spinlock.lock = 0;

    return AE_OK;
}

void AcpiOsDeleteMutex(ACPI_MUTEX Handle)
{
    MmFreeKernelHeap(Handle);
}

ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout)
{
    if(NULL == Handle)
        return AE_BAD_PARAMETER;
    
    uint64_t time;
    if(0 == Timeout)
        time = KE_MUTEX_NO_WAIT;
    else if(0xFFFF == Timeout)
        time = KE_MUTEX_NORMAL;
    else
        time = MS_TO_NS(Timeout);

    if(KeAcquireMutexWithTimeout(Handle, time))
        return AE_OK;
    else
        return AE_TIME;
}

void AcpiOsReleaseMutex(ACPI_MUTEX Handle)
{
    KeReleaseMutex(Handle);
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle)
{
    *OutHandle = MmAllocateKernelHeap(sizeof(KeSemaphore));
    if(NULL == *OutHandle)
        return AE_NO_MEMORY;
    
    (*OutHandle)->max = MaxUnits;
    (*OutHandle)->current = InitialUnits;
    (*OutHandle)->queueBottom = NULL;
    (*OutHandle)->queueTop = NULL;
    (*OutHandle)->spinlock.lock = 0;

    return AE_OK;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle)
{
    MmFreeKernelHeap(Handle);
    return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout)
{
    if(NULL == Handle)
        return AE_BAD_PARAMETER;
    
    uint64_t time;
    if(0 == Timeout)
        time = KE_MUTEX_NO_WAIT;
    else if(0xFFFF == Timeout)
        time = KE_MUTEX_NORMAL;
    else
        time = MS_TO_NS(Timeout);

    //FIXME: this implementation is incorrect
    //the timeout should be counted starting from the first unit
    UINT32 i = 0;
    bool status = true;
    for(; i < Units; i++)
    {
        if(false == (status = KeAcquireSemaphoreWithTimeout(Handle, time)))
            break;
    }
    if(false == status)
    {
        for(; i > 0; i--)
        {
            KeReleaseSemaphore(Handle);
        }
        return AE_TIME;
    }
    return AE_OK;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units)
{
    if(NULL == Handle)
        return AE_BAD_PARAMETER;

    for(; Units > 0; Units--)
    {
        KeReleaseSemaphore(Handle);
    }
    return AE_OK;
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle)
{
    *OutHandle = MmAllocateKernelHeap(sizeof(KeSpinlock));
    (*OutHandle)->lock = 0;
    if(NULL == *OutHandle)
        return AE_NO_MEMORY;

    return AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{
    MmFreeKernelHeap(Handle);
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
    KeAcquireSpinlock(Handle);
    return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
    KeReleaseSpinlock(Handle);
}

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void *Context)
{   
    STATUS ret = HalRegisterIrq(InterruptLevel, Handler, Context, 
        (struct HalInterruptParams){.mode = IT_MODE_FIXED, .polarity = IT_POLARITY_ACTIVE_LOW, .trigger = IT_TRIGGER_LEVEL, 
            .wake = IT_WAKE_CAPABLE, .shared = IT_NOT_SHAREABLE});
    
    if(OK == ret)
        HalEnableIrq(InterruptLevel, Handler);

    if(IT_ALREADY_REGISTERED == ret)
        return AE_ALREADY_EXISTS;
    else if(OK != ret)
        return AE_BAD_PARAMETER;
    else 
        return AE_OK;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber, ACPI_OSD_HANDLER Handler)
{
    HalDisableIrq(InterruptNumber, Handler);
    STATUS ret = HalUnregisterIrq(InterruptNumber, Handler);

    if(IT_NOT_REGISTERED == ret)
        return AE_NOT_EXIST;
    else if(OK != ret)
        return AE_BAD_PARAMETER;
    else 
        return AE_OK;
}

/* 
Are these functions even needed? No real implementation
in ACPICA examples.
*/

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width)
{
    uint64_t *mem = MmMapMmIo(Address, MM_PAGE_SIZE);
    if(NULL == mem)
    {
        return AE_NO_MEMORY;
    }
    
    switch(Width)
    {
        case 8:
            *Value = *((uint8_t*)mem);
            break;
        case 16:
            *Value = *((uint16_t*)mem);
            break;
        case 32:
            *Value = *((uint32_t*)mem);
            break;
        case 64:
            *Value = *((uint64_t*)mem);
            break;
        default:
            MmUnmapMmIo(mem);
            return AE_BAD_PARAMETER;
            break;
    }
    MmUnmapMmIo(mem);
    return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width)
{
    uint64_t *mem = MmMapMmIo(Address, MM_PAGE_SIZE);
    if(NULL == mem)
    {
        return AE_NO_MEMORY;
    }
    
    switch(Width)
    {
        case 8:
            *((uint8_t*)mem) = (uint8_t)Value; 
            break;
        case 16:
            *((uint16_t*)mem) = (uint16_t)Value; 
            break;
        case 32:
            *((uint32_t*)mem) = (uint32_t)Value; 
            break;
        case 64:
            *((uint64_t*)mem) = (uint64_t)Value; 
            break;
        default:
            MmUnmapMmIo(mem);
            return AE_BAD_PARAMETER;
            break;
    }
    MmUnmapMmIo(mem);
    return AE_OK;
}

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width)
{
    switch(Width)
    {
        case 8:
            *Value = IoPortReadByte(Address); 
            break;
        case 16:
            *Value = IoPortReadWord(Address); 
            break;
        case 32:
            *Value = IoPortReadDWord(Address); 
            break;
        default:
            return AE_BAD_PARAMETER;
            break;
    }
    return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{
    switch(Width)
    {
        case 8:
            IoPortWriteByte(Address, Value); 
            break;
        case 16:
            IoPortWriteWord(Address, Value); 
            break;
        case 32:
            IoPortWriteDWord(Address, Value); 
            break;
        default:
            return AE_BAD_PARAMETER;
            break;
    }
    return AE_OK;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info)
{
    switch(Function)
    {
        case ACPI_SIGNAL_FATAL:
            ACPI_SIGNAL_FATAL_INFO *s = (ACPI_SIGNAL_FATAL_INFO*)Info;
            KePanicEx(DRIVER_FATAL_ERROR, s->Type, s->Code, s->Argument, 0);
        default:
            break;
    }
    return AE_OK;
}

UINT64 AcpiOsGetTimer(void)
{
    return HalGetTimestamp() / 10;
}

ACPI_STATUS AcpiOsGetLine(char *Buffer, UINT32 BufferLength, UINT32 *BytesRead)
{
    *BytesRead = 0;
    return AE_OK;
}

ACPI_PRINTF_LIKE(1)
void ACPI_INTERNAL_VAR_XFACE AcpiOsPrintf(const char *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    // if(NULL != AcpiLogHandle)
    //     IoWriteSyslogV(AcpiLogHandle, SYSLOG_INFO, Format, Args);
    va_end(Args);
}

void AcpiOsVprintf(const char *Format, va_list Args)
{
    // if(NULL != AcpiLogHandle)
    //     IoWriteSyslogV(AcpiLogHandle, SYSLOG_INFO, Format, Args);
}

void AcpiOsRedirectOutput(void *Destination)
{

}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 *Value, UINT32 Width)
{
    *Value = 0;
    return AE_OK;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 Value, UINT32 Width)
{
    return AE_OK;
}

ACPI_STATUS AcpiOsEnterSleep(UINT8 SleepState, UINT32 RegaValue, UINT32 RegbValue)
{
    return AE_OK;
}