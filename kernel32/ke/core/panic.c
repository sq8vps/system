#include "panic.h"
#include "common.h"

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
};

void KePanicInternal(uintptr_t ip, uintptr_t code)
{
    ASM("cli");
    CmPrintf("KERNEL PANIC! Error %s (0x%X) at EIP 0x%X\n", panicStrings[code], code, ip);
    while(1);
}

void KePanicExInternal(uintptr_t ip, uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
{
    ASM("cli");
    CmPrintf("KERNEL PANIC! Error %s (0x%X) at EIP 0x%X, additional info 0x%X, 0x%X, 0x%X, 0x%X\n", panicStrings[code], code, ip, arg1, arg2, arg3, arg4);
    while(1);
}

NORETURN void KePanicFromInterrupt(char *moduleName, uintptr_t ip, uintptr_t code)
{
    ASM("cli");
    CmPrintf("KERNEL PANIC! Error %s (0x%X) at EIP 0x%X", panicStrings[code], code, ip);
    if(moduleName)
        CmPrintf(" in module %s", moduleName);
    CmPrintf("\n");
    while(1);
}

NORETURN void KePanicFromInterruptEx(char *moduleName, uintptr_t ip, uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
{
    ASM("cli");
    CmPrintf("KERNEL PANIC! Error %s (0x%X) at EIP 0x%X, additional info 0x%X, 0x%X, 0x%X, 0x%X", panicStrings[code], code, ip, arg1, arg2, arg3, arg4);
    if(moduleName)
        CmPrintf(" in module %s", moduleName);
    CmPrintf("\n");
    while(1);
}