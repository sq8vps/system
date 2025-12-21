[bits 32]
section .text

;uint32_t __nabla_do_syscall(uint32_t code, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5)
global __nabla_do_syscall:function
__nabla_do_syscall:
    push .1 ;store return address so that the kernel knows where to return
    mov ecx,[esp] ;store stack pointer so that the kernel can restore it
    mov eax,[esp + 8] ;syscall number
    mov ebx,[esp + 12] ;arg1
    mov edx,[esp + 16] ;arg2
    mov esi,[esp + 20] ;arg3
    mov edi,[esp + 24] ;arg4
    mov ebp,[esp + 28] ;arg5
    sysenter
    .1:
    add esp, 4 ;by convention, we are pushing the return address, so remove it here
    ret

;void *__nabla_get_tls(void);
global __nabla_get_tls:function
__nabla_get_tls:
    lea eax,[gs:0x0]
    ret

