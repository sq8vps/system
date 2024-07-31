;struct KeTaskControlBlock *currentTask
extern currentTask
;void *KeCurrentCpuState
extern KeCurrentCpuState

;struct KeTaskControlBlock *nextTask
extern nextTask
;void *KeNextCpuState
extern KeNextCpuState

;void KeSchedule(void)
;This is the callback function that should provide scheduling,
;that is it should set new task TCB in *nextTask
extern KeSchedule

;void GdtUpdateTss(uintptr_t esp0)
;This is the function that updates the TSS for current CPU
extern GdtUpdateTss

;void HalStoreMathState(struct KeTaskControlBlock *tcb)
extern HalStoreMathState

;void HalRestoreMathState(struct KeTaskControlBlock *tcb)
extern HalRestoreMathState

section .text

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

    mov edi,[KeCurrentCpuState]
    mov [edi + CPUState.esp],esp ;store kernel stack pointer. User mode stack pointer is on kernel stack
    add DWORD [edi + CPUState.esp],4 ;omit locally pushed EDI

    mov edi,[currentTask]

    push eax
    push edi
    call HalStoreMathState
    add esp,4
    pop eax

    pop edi ;restore original edi
    push eax ;push return address
    ret

;This function performs task switch, that is it loads and executes
;the task which TCB is stored in *nextTask.
;Also, the stack must contain all GP registers, EFLAGS, CS and EIP.
;This function does not return to it's caller.
KeSwitchToTask:
    add esp,4 ;remove return address - not needed

    mov esi,[nextTask]
    mov [currentTask],esi
    mov [nextTask],DWORD 0

    push esi
    call HalRestoreMathState
    add esp,4

    mov esi,[KeNextCpuState]
    mov [KeCurrentCpuState],esi
    mov [KeNextCpuState],DWORD 0
    mov esp,[esi + CPUState.esp] ;update kernel stack ESP

    mov eax,cr3 ;get current CR3
    mov edx,[esi + CPUState.cr3] ;get task CR3
    cmp eax,edx ;compare current CR3 and task CR3
    je .skipCR3switch ;skip if both CR3 are the same - avoid TLB flush
    mov cr3,edx

    .skipCR3switch:

    ; ;update ESP0 in TSS for privilege level switches
    ; mov eax,[esi + CPUState.esp0] 
    ; ;push arguments
    ; push eax ;esp0
    ; call GdtUpdateTss
    ; add esp,4 ;remove arguments

    ; restore segment registers
    mov ax,[esi + CPUState.ds]
    mov ds,ax
    mov ax,[esi + CPUState.es]
    mov es,ax
    mov ax,[esi + CPUState.fs]
    mov fs,ax
    mov ax,[esi + CPUState.gs]
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

;extern void HalPerformTaskSwitch(void)
;Perform task switch immediately if new task is available
;This function returns when the calling task is scheduled again
global HalPerformTaskSwitch:function
HalPerformTaskSwitch:
    mov eax,[nextTask]
    test eax,0xFFFFFFFF
    jz .returnFromSwitch

    sub esp,12 ;make room for EIP, CS and EFLAGS

    ;store EIP
    mov [esp],DWORD .returnFromSwitch
    ;store CS
    xor eax,eax
    mov ax,cs
    mov [esp+4],eax
    ;store EFLAGS
    pushfd
    pop eax
    mov [esp+8],eax

    ;store task context immediately
    call KeStoreTaskContext

    ;new task pointer is in nextTask
    ;switch to the new task
    call KeSwitchToTask

    ;this point should be unreachable
    jmp $

.returnFromSwitch: ;but return here on task switch
    ret


struc CPUState
    .esp resd 1
    .esp0 resd 1

    .cr3 resd 1

    .ds resw 1
    .es resw 1
    .fs resw 1
    .gs resw 1
endstruc