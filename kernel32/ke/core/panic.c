#include "panic.h"
#include "sdrv/bootvga/bootvga.h"
#include "ex/kdrv.h"
#include "it/it.h"

#define PANIC_STRING(code) [code] = STRINGIFY(code)

static const char *panicStrings[] =
{
    PANIC_STRING(NON_MASKABLE_INTERRUPT),
    PANIC_STRING(NON_MASKABLE_INTERRUPT),
    PANIC_STRING(DIVISION_BY_ZERO),
    PANIC_STRING(INVALID_OPCODE),
    PANIC_STRING(DOUBLE_FAULT),
    PANIC_STRING(GENERAL_PROTECTION_FAULT),
    PANIC_STRING(BOOT_FAILURE),
    PANIC_STRING(NO_EXECUTABLE_TASK),
    PANIC_STRING(UNACQUIRED_MUTEX_RELEASED),
    PANIC_STRING(BUSY_MUTEX_ACQUIRED),
    PANIC_STRING(PAGE_FAULT),
    PANIC_STRING(MACHINE_CHECK_FAULT),
    PANIC_STRING(UNEXPECTED_FAULT),
    PANIC_STRING(ACPI_FATAL_ERROR),
    PANIC_STRING(PRIORITY_LEVEL_TOO_LOW),
    PANIC_STRING(PRIORITY_LEVEL_TOO_HIGH),
    PANIC_STRING(RP_FINALIZED_OUT_OF_LINE),
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
    struct ExDriverObject *t = ExFindDriverByAddress(ip);
    if(NULL != t)
        BootVgaPrintString(t->fileName);
    else
        BootVgaPrintChar('?');

    BootVgaPrintString(" (IP: ");
    printHex(ip);
    BootVgaPrintString(")\n\n");
}

NORETURN void KePanicInternal(uintptr_t ip, uintptr_t code)
{
    ItDisableInterrupts();
    printPanic(ip, code);
    while(1)
        ;
}

NORETURN void KePanicExInternal(uintptr_t ip, uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
{
    ItDisableInterrupts();
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

NORETURN void KePanicIP(uintptr_t ip, uintptr_t code)
{
    KePanicInternal(ip, code);
}

NORETURN void KePanicIPEx(uintptr_t ip, uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
{
    KePanicExInternal(ip, code, arg1, arg2, arg3, arg4);
}