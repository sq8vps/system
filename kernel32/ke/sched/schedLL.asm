;struct KeTaskControlBlock *currentTask
extern currentTask

;void KeSchedule(void)
;This is the callback function that should provide scheduling,
;that is it should set new task TCB in *currentTask
extern KeSchedule

;void KeUpdateTSS(uint16_t cpu, uintptr_t esp0)
;This is the function that updates the TSS for given CPU
extern KeUpdateTSS

;void KeSchedulerISRClearFlag()
;This function is a wrapper that cleans appropriate interrupt flag
extern KeSchedulerISRClearFlag

;uint32_t KeSchedPostponeCounter;
extern KeSchedPostponeCounter
;bool KeSchedTaskSwitchPending
extern KeSchedTaskSwitchPending

section .text

;void KeSchedulerISR(struct ItFrame *f)
global KeSchedulerISR
KeSchedulerISR:
    test DWORD [KeSchedPostponeCounter],0xFFFFFFFF ;check if task switches are postponed
    jz .continueScheduling 
    ;if so, mark task switch is pending and return to previous task
    mov BYTE [KeSchedTaskSwitchPending],1
    iret

    .continueScheduling:

    ;store task context immediately
    call KeStoreTaskContext

    ;invoke scheduler
    call KeSchedule

    ;new task pointer is in currentTask
    ;switch to the new task
    call KeSwitchToTask

    ;this point should be unreachable
    jmp $

;Store current task context on stack
;This function alters the stack
KeStoreTaskContext:
    xchg eax,[esp] ;exchange return address with eax
    ;now the original eax is on the stack and we have return address in eax
    ;store other general purpose registers
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp

    push edi ;save original edi

    mov edi,[currentTask]
    mov [edi + TCB.esp],esp ;store kernel stack pointer. User mode stack pointer is on kernel stack
    add DWORD [edi + TCB.esp],4 ;omit locally pushed EDI

    pop edi ;restore original edi
    push eax ;push return address
    ret

;This function performs task switch, that is it loads and executes
;the task which TCB is stored in *currentTask.
;Also, the stack must contain all GP registers, EFLAGS, CS and EIP.
;This function does not return to it's caller.
KeSwitchToTask:
    add esp,4 ;remove return address - not needed

    mov esi,[currentTask]
    mov esp,[esi + TCB.esp] ;update kernel stack ESP

    mov eax,cr3 ;get current CR3
    mov edx,[esi + TCB.cr3] ;get task CR3
    cmp eax,edx ;compare current CR3 and task CR3
    je .skipCR3switch ;skip if both CR3 are the same - avoid TLB flush
    mov cr3,edx

    .skipCR3switch:

    ;update ESP0 in TSS for privilege level switches
    mov eax,[esi + TCB.esp0] 
    ;push arguments
    push eax ;esp0
    push 0 ;cpu number
    call KeUpdateTSS
    add esp,8 ;remove arguments

    ;clear interrupt flag (send End Of Interrupt)
    call KeSchedulerISRClearFlag

    ; restore segment registers
    mov ax,[esi + TCB.ds]
    mov ds,ax
    mov ax,[esi + TCB.es]
    mov es,ax
    mov ax,[esi + TCB.fs]
    mov fs,ax
    mov ax,[esi + TCB.gs]
    mov gs,ax 

    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx
    pop eax
    ;jump to the new task
    iret

;extern void KeTaskYield(void)
;Pause task execution and yield to scheduler.
;The function returns when the task is scheduled again.
global KeTaskYield:function
KeTaskYield:
    sub esp,12 ;make room for EIP, CS and EFLAGS

    ;store EIP
    mov [esp],DWORD .returnFromYield
    ;store CS
    xor eax,eax
    mov ax,cs
    mov [esp+4],eax
    ;store EFLAGS
    pushfd
    pop eax
    mov [esp+8],eax

    cli

    ;jump to scheduler ISR as if it was end of the time slice
    jmp KeSchedulerISR

.returnFromYield: ;but return here on task switch
    ret


struc TCB
    .esp resd 1
    .esp0 resd 1

    .cr3 resd 1

    .ds resw 1
    .es resw 1
    .fs resw 1
    .gs resw 1
endstruc