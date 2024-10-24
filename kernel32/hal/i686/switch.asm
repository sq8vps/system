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

;__attribute__((fastcall))
;void GdtUpdateTss(uintptr_t esp0)
;This is the function that updates the TSS for current CPU
extern GdtUpdateTss

;__attribute__((fastcall))
;void HalStoreMathState(struct KeTaskControlBlock *tcb)
extern HalStoreMathState

;__attribute__((fastcall))
;void HalRestoreMathState(struct KeTaskControlBlock *tcb)
extern HalRestoreMathState

;__attribute__((fastcall))
;void KeAttachLastTask(uint16_t cpu)
extern KeAttachLastTask

;volatile bool KeTaskSwitchPending[MAX_CPU_COUNT] - SMP systems
;volatile bool KeTaskSwitchPending - UP systems
extern KeTaskSwitchPending

;volatile bool KeTaskSwitchInProgress[MAX_CPU_COUNT] - SMP systems
;volatile bool KeTaskSwitchInProgress - UP systems
extern KeTaskSwitchInProgress

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
    test edi,edi
    jz .skip

    mov [edi + CPUState.esp],esp ;store kernel stack pointer. User mode stack pointer is on kernel stack
    add DWORD [edi + CPUState.esp],4 ;omit locally pushed EDI

    mov edi,[KeCurrentTask+4*edx] ;store current task in last task
    mov [KeLastTask+4*edx],edi

    push eax

    cld
    mov ecx,edi ;fastcall
    call HalStoreMathState

    pop eax


.skip:
    pop edi ;restore original edi
    push eax ;push return address
    mov eax,[esp+28] ;return CPU number
    ret

;This function performs task switch, that is, it loads and executes
;the task which TCB is stored in *nextTask.
;Also, the stack must contain all GP registers, EFLAGS, CS and EIP.
;This function must be called using jmp!
KeSwitchToTask:

    mov edi,eax ;get CPU number
    
    mov esi,[KeNextTask+4*edi]
    mov [KeCurrentTask+4*edi],esi
    mov [KeNextTask+4*edi],DWORD 0

    cld
    mov ecx,esi ;fastcall
    call HalRestoreMathState

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

    ;update ESP0 in TSS for privilege level switches
    mov ecx,[esi + CPUState.esp0] 
    ;GdtUpdateTss uses fastcall
    call GdtUpdateTss

    ; restore segment registers
    mov ax,[esi + CPUState.ds]
    mov ds,ax
    mov ax,[esi + CPUState.es]
    mov es,ax
    mov ax,[esi + CPUState.fs]
    mov fs,ax
    mov ax,[esi + CPUState.gs]
    mov gs,ax 

    cli
    mov byte [KeTaskSwitchInProgress+edi],0
    sti

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

%ifdef SMP
    str eax ;get TSS offset from task register
    shr eax,3 ;divide by 8 to obtain selector number from selector offset
    sub eax,5 ;subtract 5 to get CPU number from the selector number
%else
    mov eax,0
%endif

    mov dl,[KeTaskSwitchPending+eax]
    test dl,dl
    jz .returnFromSwitch

    cli
    mov byte [KeTaskSwitchPending+eax],0

    mov edx,[KeNextTask+4*eax]
    test edx,edx ;check if there is a next task
    ;if not, then continue with the current one
    jz .returnFromSwitch

    mov byte [KeTaskSwitchInProgress+eax],1
    sti

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
    or edx,(1 << 9) ;add IF bit
    mov [esp+8],edx

    ;store task context immediately
    ;CPU number is in EAX
    call KeStoreTaskContext

    push eax

    cld
    mov ecx,eax ;fastcall
    call KeAttachLastTask

    pop eax

    ;new task pointer is in nextTask
    ;switch to the new task
    ;pass CPU number in EAX
    jmp KeSwitchToTask

    ;this point should be unreachable
    jmp $

.returnFromSwitch: ;but return here on task switch
    sti
    ret


;NORETURN void I686StartUserTask(uint16_t ds, uintptr_t esp, uint16_t cs, void (*entry)())
global I686StartUserTask:function
I686StartUserTask:
    ;build stack for iret
    sub esp,20
    mov eax,[esp + 24] ;data/stack segment selector
    mov [esp + 16],eax
    mov ds,ax
    mov es,ax
    mov fs,ax
    mov gs,ax
    mov eax,[esp + 28] ;stack pointer
    mov [esp + 12],eax
    mov eax,(1 << 1) | (1 << 9) ;eflags (reserved, interrupt enable)
    mov [esp + 8],eax
    mov eax,[esp + 32] ;code segment selector
    mov [esp + 4],eax
    mov eax,[esp + 36] ;entry point (EIP)
    mov [esp],eax

    iret

    jmp $

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