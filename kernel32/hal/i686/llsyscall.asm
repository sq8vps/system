[bits 32]

;void I686VerifySyscall(uintptr_t ret, uintptr_t stack)
extern I686VerifySyscall

;INTERNAL STATUS KePerformSyscall(uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5)
extern KePerformSyscall

global I686Sysenter:function
I686Sysenter:
    sti
    ;on sysenter, the CPU sets up kernel CS, SS, ESP and EIP
    ;user code is expected to:
    ;1. push the return address on the stack
    ;2. store stack pointer (ESP) in ECX
    ;3. store call code in EAX
    ;4. store additional arguments in registers: ebx, edx, esi, edi, ebp in this order
    
    push ecx ;store user mode ESP for our use

    ;push arguments on the stack so we can call the C function
    push ebp
    push edi
    push esi
    push edx
    push ebx
    push eax
    push ecx

    mov eax,[ecx] ;get return address provided by the caller
    push eax ;store return address
    
    ;verify return address and stack address
    ;this function should terminate the task if any of these is bad
    cld
    call I686VerifySyscall
    add esp,(2 * 4)

    ;perform actual system call
    cld
    call KePerformSyscall
    ;return code should be in EAX
    add esp,(6 * 4)

    pop ecx ;restore user mode ESP
    mov edx,[ecx] ;get return address from user stack
    ;ESP is ECX and EIP is in EDX, now exit kernel mode
    sysexit





