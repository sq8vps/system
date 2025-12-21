#include "panic.h"
#include "hal/video.h"
#include "ex/kdrv/kdrv.h"
#include "it/it.h"
#include "hal/cpu.h"
#include "rtl/stdio.h"
#include "hal/debug.h"

#define PANIC_STRING(code) [code] = STRINGIFY(code)

static const char *KePanicStrings[] =
{
    PANIC_STRING(NO_ERROR),
    PANIC_STRING(KERNEL_MODE_FAULT),
    PANIC_STRING(BOOT_FAILURE),
    PANIC_STRING(NO_EXECUTABLE_TASK),
    PANIC_STRING(UNACQUIRED_MUTEX_RELEASED),
    PANIC_STRING(BUSY_MUTEX_ACQUIRED),
    PANIC_STRING(UNEXPECTED_FAULT),
    PANIC_STRING(DRIVER_FATAL_ERROR),
    PANIC_STRING(PRIORITY_LEVEL_TOO_LOW),
    PANIC_STRING(PRIORITY_LEVEL_TOO_HIGH),
    PANIC_STRING(RP_FINALIZED_OUT_OF_LINE),
    PANIC_STRING(ILLEGAL_PRIORITY_LEVEL_CHANGE),
    PANIC_STRING(ILLEGAL_PRIORITY_LEVEL),
    PANIC_STRING(OBJECT_LOCK_UNAVAILABLE),
    PANIC_STRING(MEMORY_ACCESS_VIOLATION),
    PANIC_STRING(INVALID_TASK_ATTACHMENT_ATTEMPT),
};

static bool KeInPanicState = false; /**< Remember whether the kernel is already panicking to avoid panic during panic */

static void KePrintMainPanic(uintptr_t ip, uintptr_t code)
{
    char buffer[128];
    const char *codeString = "????";
    const char *moduleString = "unknown";

    if(code < (sizeof(KePanicStrings) / sizeof(*KePanicStrings)))
        codeString = KePanicStrings[code];

    uintptr_t addr = ip;
    struct ExDriverObject *t = ExFindDriverByAddress(&addr);
    if((NULL != t) && (NULL != t->imageName))
        moduleString = t->imageName;
    
    HalVideoPrint("KERNEL PANIC!\n\n");
    HalDebugPutString("KERNEL PANIC!\n\n");
    HalVideoPrint(codeString);
    HalDebugPutString(codeString);
    snprintf(buffer, sizeof(buffer), 
        " (0x%p)\n\n"
        "Failing IP: 0x%p\n\n", 
        (void*)code, (void*)ip);
    HalVideoPrint(buffer);
    HalDebugPutString(buffer);
    snprintf(buffer, sizeof(buffer), "Module: %s (base at 0x%p)\n\n", moduleString, (void*)addr);
    HalVideoPrint(buffer);
    HalDebugPutString(buffer);
}

static void KePanicStopSystem(void)
{
    HalHaltAllCpus();
    HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_HIGHEST);
    HalVideoInit();
}

NORETURN static void KePanicInternal(uintptr_t ip, uintptr_t code)
{
    if(KeInPanicState)
    {
        while(1)
            HALT();
    }
    KeInPanicState = true;
    KePanicStopSystem();
    KePrintMainPanic(ip, code);
    while(1)
        ;
}

NORETURN static void KePanicExInternal(uintptr_t ip, uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
{
    char buffer[100];
    if(KeInPanicState)
    {
        while(1)
            HALT();
    }
    KeInPanicState = true;
    KePanicStopSystem();
    KePrintMainPanic(ip, code);
    snprintf(buffer, 100, "Additional informations: 0x%p, 0x%p, 0x%p, 0x%p\n", (void*)arg1, (void*)arg2, (void*)arg3, (void*)arg4);
    HalVideoPrint(buffer);
    HalDebugPutString(buffer);
    while(1)
        ;
}


NORETURN void KePanic(uintptr_t code)
{
    KePanicInternal((uintptr_t)__builtin_extract_return_addr(__builtin_return_address (0)), code);
}

NORETURN void KePanicEx(uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
{
    KePanicExInternal((uintptr_t)__builtin_extract_return_addr(__builtin_return_address (0)), code, 
        arg1, arg2, arg3, arg4);
}

NORETURN void KePanicIP(uintptr_t ip, uintptr_t code)
{
    KePanicInternal(ip, code);
}

NORETURN void KePanicIPEx(uintptr_t ip, uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
{
    KePanicExInternal(ip, code, arg1, arg2, arg3, arg4);
}