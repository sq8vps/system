#include "panic.h"
#include "common.h"

EXPORT_SYMBOL(KePanic);
EXPORT_SYMBOL(KePanicEx);
EXPORT_SYMBOL(KePanicFromInterrupt);
EXPORT_SYMBOL(KePanicFromInterruptEx);

void KePanicInternal(uintptr_t ip, uintptr_t code)
{
    ASM("cli");
    CmPrintf("KERNEL PANIC! Error 0x%X at EIP 0x%X\n", code, ip);
    while(1);
}

void KePanicExInternal(uintptr_t ip, uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
{
    ASM("cli");
    CmPrintf("KERNEL PANIC! Error 0x%X at EIP 0x%X, additional info 0x%X, 0x%X, 0x%X, 0x%X\n", code, ip, arg1, arg2, arg3, arg4);
    while(1);
}

NORETURN void KePanicFromInterrupt(char *moduleName, uintptr_t ip, uintptr_t code)
{
    ASM("cli");
    CmPrintf("KERNEL PANIC! Error 0x%X at EIP 0x%X", code, ip);
    if(moduleName)
        CmPrintf(" in module %s", moduleName);
    CmPrintf("\n");
    while(1);
}

NORETURN void KePanicFromInterruptEx(char *moduleName, uintptr_t ip, uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
{
    ASM("cli");
    CmPrintf("KERNEL PANIC! Error 0x%X at EIP 0x%X, additional info 0x%X, 0x%X, 0x%X, 0x%X", code, ip, arg1, arg2, arg3, arg4);
    if(moduleName)
        CmPrintf(" in module %s", moduleName);
    CmPrintf("\n");
    while(1);
}