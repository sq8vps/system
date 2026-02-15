[bits 32]

;keep this synchronized with defines.h!!!!
MAX_ARG_COUNT equ 10

;STATUS I686ValidateSyscall(reg_t stack, reg_t code, size_t *argSize)
extern I686ValidateSyscall

;INTERNAL reg_t KePerformSyscall(reg_t code, ...)
extern KePerformSyscall

;void KeValidateUserReturnAddress(reg_t ret)
extern KeValidateUserReturnAddress

;const struct KeSyscallDescriptor* KeGetSyscallDescriptor(size_t code)
extern KeGetSyscallDescriptor

global I686Sysenter:function
I686Sysenter:
    sti
    push ds
    push es
    ;on sysenter, the CPU sets up kernel CS, SS, ESP and EIP
    ;user code is expected to:
    ;1. If there are more than 20 bytes of arguments, push all function arguments on the stack, last argument pushed first
    ;2. Push the return address on the stack
    ;3. Store stack pointer (ESP), that should be set on return from "sysenter", in EBP
    ;4. Store system call code in EAX
    ;5. If there are at most 20 bytes of arguments, store function arguments in registers: ebx, ecx, edx, esi, edi in this order. 
    ;    For more than 20 bytes of arguments, see point 1.
    ;note 1: all arguments are 32-bits-aligned. If an argument is shorter, then it is padded with zeros to 32 bits.
    ;note 2: if an argument is bigger than 32 bits, then it is split to 32 bit parts starting from the least significant 32-bit word and:
    ;   a. if there are enough free registers, the parts are stored in the registers in the order specified in point 5, *least* significant part first,
    ;   b. if there are no not enough free registers, the parts are pushed onto the stack, *most* significant part first.
    ;note 3: no registers are preserved on "sysenter"
    ;note 4: last 32-bit dword pushed on the stack should be the return address, 
    ;    i.e., EBP should point to the bottom of user mode stack and dereferencing EBP (dword [EBP]) should result in 32-bit return address
    
    ;prepare space for:
    ;1. total size of arguments at the very top
    ;2. potential space for the additional user arguments that may need to be copied
    sub esp,((1 + MAX_ARG_COUNT - 5) * 4)
    ;push user arguments on the stack
    push edi
    push esi
    push edx
    push ecx
    push ebx
    ;store syscall code. EBX should be preserved
    mov ebx,eax

    ;User mode stack pointer is in ebp and should be preserved by callees
 
    ;get function local space as total argument size storage
    lea edx,[esp + 4 * (5 + (MAX_ARG_COUNT - 5))]
    push edx
    ;push syscall code
    push eax
    ;push stack pointer
    push ebp
    ;verify that the syscall code is OK, the stack is OK, and that there are enough arguments for the call
    ;I686ValidateSyscall() should terminate task if user stack is bad
    cld
    call I686ValidateSyscall
    add esp,(3 * 4)
    test eax,eax ;if not OK, then return to the user and pass error code
    jnz .end 
    ;if OK, then we should have the total size of arguments at [esp + 4 * (6 + (MAX_ARG_COUNT - 5))] -> ecx
    mov ecx,[esp + 4 * (5 + (MAX_ARG_COUNT - 5))]
    cmp ecx,20 ;register can hold 5 arguments, so 20 bytes
    jle .perform ;all fits in the registers, just perform system call
    ;if does not fit, then we need to copy arguments from user stack to kernel stack
    shr ecx,2 ;get number of dwords
    mov ax,32 ;source segment is user mode data
    mov ds,ax 
    mov ax,16 ;destination segment is kernel mode data 
    mov es,ax 
    lea esi,[ebp + 4] ;get user stack pointer
    mov edi,esp ;get kernel stack pointer, replace previously pushed registers, because they weren't used anyway
    cld ;incremented edi and esi
    rep movsd ;copy ecx dwords from esi to edi

    .perform:
    ;perform actual system call
    push ebx ;get syscal code from ebx, that should be preserved
    call KeGetSyscallDescriptor
    ;EAX should contain a pointer to the syscall descriptor
    ;the first element of the syscall descriptor is the routine address
    add esp,4
    cld
    call [eax]
    
    .end:
    ;return code is in eax, but move it to ebx (eax is not preserved)
    mov ebx,eax
    ;remove all previously pushed arguments
    add esp,(4 * (1 + MAX_ARG_COUNT))

    ;push return address
    push dword [ebp]
    cld
    ;Check if return address is ok. If not, then the task is terminated.
    call KeValidateUserReturnAddress
    add esp,4

    mov eax,ebx ;restore return code
    mov ecx,ebp ;restore user mode ESP
    mov edx,[ebp] ;get return address from user stack
    ;ESP is ECX and EIP is in EDX, now exit kernel mode
    pop es
    pop ds
    sysexit
