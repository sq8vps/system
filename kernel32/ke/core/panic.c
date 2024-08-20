#include "panic.h"
#include "hal/i686/bootvga/bootvga.h"
#include "ex/kdrv/kdrv.h"
#include "it/it.h"
#include "hal/arch.h"

#define PANIC_STRING(code) [code] = STRINGIFY(code)

static const char *panicStrings[] =
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
    PANIC_STRING(INVALID_TASK_ATTACHMENT_ATTEMPT),
};

static void printHex(uintptr_t x)
{
    BootVgaPrintString("0x");
    uint8_t pos = (sizeof(uintptr_t) * 8) - 4;
    for(uint8_t i = 0; i < (sizeof(uintptr_t) * 2); i++)
    {
        uint8_t val = (x >> pos) & 0xF;
        if(val < 10)
            BootVgaPrintChar(val + '0');
        else
            BootVgaPrintChar(val - 10 + 'A');
        pos -= 4;
    }
}

static void printPanic(uintptr_t ip, uintptr_t code)
{
    BootVgaPrintString("KERNEL PANIC!\n\n\n");
    BootVgaPrintString((char*)panicStrings[code]);
    BootVgaPrintString(" (");
    printHex(code);
    BootVgaPrintString(")\n\nModule: ");
    uintptr_t addr = ip;
    struct ExDriverObject *t = ExFindDriverByAddress(&addr);
    if(NULL != t)
    {
        //BootVgaPrintString(t->fileName);
        BootVgaPrintString("??, base at: ");
        printHex(addr);
    }
    else
        BootVgaPrintChar('?');

    BootVgaPrintString(" (IP: ");
    printHex(ip);
    BootVgaPrintString(")\n\n");
}

NORETURN void KePanicInternal(uintptr_t ip, uintptr_t code)
{
    HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_HIGHEST);
    HalHaltAllCpus();
    printPanic(ip, code);
    while(1)
        ;
}

NORETURN void KePanicExInternal(uintptr_t ip, uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
{
    HalRaisePriorityLevel(HAL_PRIORITY_LEVEL_HIGHEST);
    HalHaltAllCpus();
    printPanic(ip, code);
    BootVgaPrintString("Additional informations: ");
    printHex(arg1);
    BootVgaPrintString(", ");
    printHex(arg2);
    BootVgaPrintString(", ");
    printHex(arg3);
    BootVgaPrintString(", ");
    printHex(arg4);
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