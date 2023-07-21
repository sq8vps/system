;void KePanicInternal(uintptr_t ip, uintptr_t code)
extern KePanicInternal
;void KePanicExInternal(uintptr_t ip, uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
extern KePanicExInternal

;void KePanic(uintptr_t code)
global KePanic
KePanic:
    ;the stack should look like:
    ;...
    ;error code
    ;return address
    ;Use previous return address and error code as arguments for KePanicInternal()
    call KePanicInternal
    jmp $

;void KePanicEx(uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4)
global KePanicEx
KePanicEx:
    ;same as above, except there are more arguments
    call KePanicExInternal
    jmp $
