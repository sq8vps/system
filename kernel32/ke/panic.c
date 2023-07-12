#include "panic.h"

#include "../../drivers/vga/vga.h"


NORETURN void KePanic(uintptr_t code)
{
    printf("KERNEL PANIC! Error 0x%X\n", code);
    while(1);;
}

NORETURN void KePanicEx(uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
{
    printf("KERNEL PANIC! Error 0x%X, additional info 0x%x, 0x%X, 0x%X, 0x%X\n", code, arg1, arg2, arg3, arg4);
    while(1);;
}