[bits 32]
section .text

;All functions must be position independent!

;reg_t __ApiDoSyscall(reg_t argSize, reg_t code, ...)
global __ApiDoSyscall:function
__ApiDoSyscall:
    push ebx
    push esi
    push edi
    push ebp

    mov ecx,[esp + 5 * 4] ;get argSize
    cmp ecx,20 ;check if there are at most 20 bytes of arguments
    ja .doStack ;if not, don't bother with registers, but there is some magic that needs to be done
    ;if so, move arguments to registers

    ;we need to calculate the return address dynamically
    call .1
    .1: ;now, we have the address of the instruction below on the stack
    add dword [esp],.retRegs - $ ;calculate absolute return address from the address of this instruction and the relative displacement

    mov eax,[esp + 6 * 4] ;syscall number
    mov ebx,[esp + 7 * 4] ;arg1
    mov ecx,[esp + 8 * 4] ;arg2
    mov edx,[esp + 9 * 4] ;arg3
    mov esi,[esp + 10 * 4] ;arg4
    mov edi,[esp + 11 * 4] ;arg5
    mov ebp,esp ;store stack pointer
    sysenter ;perform a syscall
    ;we never return here, but to .retRegs

    .doStack:
    ;store call code in eax
    mov eax,[esp + 6 * 4]
    ;to minimize copying, we directly pass our arguments to the kernel
    ;however, the last element on the stack must be the return address
    ;we can replace syscall code with our return address and then pass our stack directly
    
    ;we need to calculate the return address dynamically
    call .2
    .2: ;now, we have the address of the instruction below on the stack
    pop eax ;move address of this instruction to eax - this instruction takes 2 bytes
    add eax,.retStack - $ + 1 ;calculate absolute return address from the address of the instruction above and the relative displacement
    mov [esp + 6 * 4],eax ;replace syscall code with the return address
    lea ebp,[esp + 6 * 4] ;store "fake" stack pointer
    sysenter ;perform a syscall

    .retStack:
    sub esp,6 * 4 ;realign stack
    jmp .end

    .retRegs:
    add esp,4 ;by convention, we are pushing the return address, so remove it here

    .end:
    pop ebp
    pop edi
    pop esi
    pop ebx
    cld
    ret