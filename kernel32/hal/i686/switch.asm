;struct KeTaskControlBlock *KeCurrentTask[]
extern KeCurrentTask
;void *KeCurrentCpuState[]
extern KeCurrentCpuState

;struct KeTaskControlBlock *KeNextTask[]
extern KeNextTask
;void *KeNextCpuState[]
extern KeNextCpuState

;struct KeTaskControlBlock *KeLastTask[]
extern KeLastTask

;void GdtUpdateTss(uintptr_t esp0)
;This is the function that updates the TSS for current CPU
extern GdtUpdateTss

;void HalStoreMathState(struct KeTaskControlBlock *tcb)
extern HalStoreMathState

;void HalRestoreMathState(struct KeTaskControlBlock *tcb)
extern HalRestoreMathState

;void KeAttachLastTask(uint16_t cpu)
extern KeAttachLastTask

;volatile bool KeTaskSwitchPending[MAX_CPU_COUNT] - SMP systems
;volatile bool KeTaskSwitchPending - UP systems
extern KeTaskSwitchPending

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

    push edi ;store edi temporarily

    mov edx,[esp+28] ;get cpu number, which is on the stack

    mov edi,[KeCurrentCpuState+4*edx]
    ;check if KeCurrentCpuState is NULL
    ;if so, we are probably just starting and the currently executed code is not a task
    ;just drop all data and switch to the next task, the scheduler is aware of it
    test edi,0xFFFFFFFF
    jz .skip

    mov [edi + CPUState.esp],esp ;store kernel stack pointer. User mode stack pointer is on kernel stack
    add DWORD [edi + CPUState.esp],4 ;omit locally pushed EDI

    mov edi,[KeCurrentTask+4*edx] ;store current task in last task
    mov [KeLastTask+4*edx],edi

    push eax

    cld
    push edi
    call HalStoreMathState
    add esp,4

    pop eax


.skip:
    pop edi ;restore original edi
    push eax ;push return address
    mov eax,[esp+28] ;return CPU number
    ret

;This function performs task switch, that is, it loads and executes
;the task which TCB is stored in *nextTask.
;Also, the stack must contain all GP registers, EFLAGS, CS and EIP.
;This function does not return to it's caller.
KeSwitchToTask:
    add esp,4 ;remove return address - not needed

    mov edi,eax ;get CPU number
    
    mov esi,[KeNextTask+4*edi]
    mov [KeCurrentTask+4*edi],esi
    mov [KeNextTask+4*edi],DWORD 0

    cld
    push esi
    call HalRestoreMathState
    add esp,4

    mov esi,[KeNextCpuState+4*edi]
    mov [KeCurrentCpuState+4*edi],esi
    mov [KeNextCpuState+4*edi],DWORD 0
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
    cli

%ifdef SMP
    str eax ;get TSS offset from task register
    shr eax,3 ;divide by 8 to obtain selector number from selector offset
    sub eax,5 ;subtract 5 to get CPU number from the selector number
%else
    mov eax,0
%endif

    mov dl,[KeTaskSwitchPending+eax]
    test dl,0xFF
    jz .returnFromSwitch

    mov byte [KeTaskSwitchPending+eax],0

    mov edx,[KeNextTask+4*eax]
    test edx,0xFFFFFFFF ;check if there is a next task
    ;if not, then continue with the current one
    jz .returnFromSwitch

    sub esp,12 ;make room for EIP, CS and EFLAGS

    ;store EIP
    mov [esp],DWORD .returnFromSwitch
    ;store CS
    xor edx,edx
    mov dx,cs
    mov [esp+4],edx
    ;store EFLAGS
    pushfd
    pop edx
    mov [esp+8],edx

    ;store task context immediately
    ;CPU number is in EAX
    call KeStoreTaskContext

    push eax

    cld
    push ax
    call KeAttachLastTask
    add esp,2

    pop eax

    ;new task pointer is in nextTask
    ;switch to the new task
    ;pass CPU number in EAX
    call KeSwitchToTask

    ;this point should be unreachable
    jmp $

.returnFromSwitch: ;but return here on task switch
    sti
    ret


struc CPUState
    .esp resd 1
    .esp0 resd 1

    .cr3 resd 1

    .ds resw 1
    .es resw 1
    .fs resw 1
    .gs resw 1

    .prio resb 1
endstruc