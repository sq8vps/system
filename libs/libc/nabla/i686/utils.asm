[bits 32]
section .text

;void *__nabla_get_tls(void);
global __nabla_get_tls:function
__nabla_get_tls:
    mov eax,[gs:0x0]
    ret

