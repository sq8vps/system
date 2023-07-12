;extern struct TaskControlBlock *currentTask
extern currentTask

;void KeSchedule(void)
;This is the callback function that should provide scheduling,
;that is it should set new task TCB in *currentTask
extern KeSchedule

extern KeUpdateTSS

extern KeSchedulerISRClearFlag

;void KeSchedulerISR(struct ItFrame *f)
global KeSchedulerISR
KeSchedulerISR:
    push eax
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp

    mov edi,[currentTask]
    mov [edi + TCB.esp],esp ;store kernel stack pointer. User mode stack pointer is on kernel stack

    xchg bx,bx

    ;invoke scheduler
    mov ebp,esp
    call KeSchedule

    ;there should be new task in *currentTask

    mov esi,[currentTask]
    mov esp,[esi + TCB.esp] ;update kernel stack ESP.
    mov eax,[esi + TCB.esp0] ;update ESP0 in TSS for privilege level switches
    mov ebp,esp
    push eax ;push arguments
    push 0
    call KeUpdateTSS
    add esp,8 ;remove arguments

    mov eax,cr3 ;get current CR3
    mov edx,[esi + TCB.cr3] ;get task CR3
    cmp eax,edx ;compare current CR3 and task CR3
    je .skipCR3switch ;skip CR3 if not different - avoid TLB flush
    mov cr3,edx

    .skipCR3switch:

    ;clear interrupt flag (send End Of Interrupt)
    mov ebp,esp
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


struc TCB
    .esp resd 1
    .esp0 resd 1

    .cr3 resd 1

    .ds resw 1
    .es resw 1
    .fs resw 1
    .gs resw 1

endstruc

; ;extern struct TaskControlBlock *currentTask
; extern currentTask

; ;void KeSchedule(void)
; ;This is the callback function that should provide scheduling,
; ;that is it should set new task TCB in *currentTask
; extern KeSchedule

; extern KeSchedulerISRClearFlag

; ;void KeSchedulerISR(struct ItFrame *f)
; global KeSchedulerISR
; KeSchedulerTimerISR:
;     ;store current task context
;     call KeStoreTaskContext

;     mov eax,[esp + 4] ;get last task CS
;     add esp,12 ;drop EIP, CS and EFLAGS from the stack
;     mov dx,cs ;get current CS
;     cmp ax,dx ;check if there was a privilege level change
;     je .skipSsEspRemoval
;     add esp,8  ;if there was, drop SS and ESP
;     .skipSsEspRemoval:

;     ;invoke scheduler
;     mov ebp,esp
;     call KeSchedule

;     sub esp,12 ;prepare space for EIP, CS and EFLAGS on the stack
;     mov esi,[currentTask] ;get new task CS
;     mov ax,[esi + TCB.cs]
;     mov dx,cs ;get current CS
;     cmp ax,dx ;check if there will be a privilege level change
;     je .skipSsEspInsertion
;     sub esp,8  ;if there will be, make room for SS and ESP
;     .skipSsEspInsertion:

;     ;clear interrupt flag (send End Of Interrupt)
;     mov ebp,esp
;     call KeSchedulerISRClearFlag

;     ;load new stack context
;     call KeLoadTaskContext

;     ;jump to the new task
;     iret



; ;Store current task context to *currentTask TCB
; ;This function assumes that it is called from the beginning of an ISR
; ;This function leaves the stack untouched
; KeStoreTaskContext:
;     push edi ; store original edi
;     mov edi,[currentTask] ;use edi as TCB pointer
;     ; store all registers
;     mov [edi + TCB.eax],eax
;     mov [edi + TCB.ebx],ebx
;     mov [edi + TCB.ecx],ecx
;     mov [edi + TCB.edx],edx
;     mov [edi + TCB.ebp],ebp
;     mov [edi + TCB.esi],esi

;     ; store registers passed on stack
;     mov eax,[esp + 8] ;store next EIP
;     mov [edi + TCB.eip],eax
;     mov eax,[esp + 12] ;store CS
;     mov [edi + TCB.cs],ax
;     mov eax,[esp + 16] ;store flags
;     mov [edi + TCB.eflags],eax

;     mov ax,cs ;get current code segment
;     cmp ax,[edi + TCB.cs] ;compare current CS with task CS
;     je .skipESPstore ;if equal then there was no privilege level change
;     ;but if there was, store original ESP and SS
;     mov eax,[esp + 20]
;     mov [edi + TCB.esp],eax
;     mov eax,[esp + 24]
;     mov [edi + TCB.ss],ax
    
;     .skipESPstore:
;     pop eax ;pop original EDI
;     mov [edi + TCB.edi],eax ;store original EDI
    
;     ret

; ;Load new task context from *currentTask
; ;This function assumes that the stack has:
; ;3 DWORDS pushed when there was no privilege change
; ;5 DWORDS pushed when there was a privilege change
; KeLoadTaskContext:
;     mov esi,[currentTask] ;use esi as TCB pointer

;     mov ax,cs ;get current code segment
;     cmp ax,[esi + TCB.cs] ;compare current CS and task CS
;     je .skipESPrestore ;if equal then there was no privilege level change
;     ;but if there was, restore the ESP and SS
;     mov eax,[esi + TCB.esp]
;     mov [esp+12],eax
;     mov ax,[esi + TCB.ss]
;     mov [esp+16],ax
    
;     .skipESPrestore:

;     mov eax,cr3 ;get current CR3
;     mov edx,[esi + TCB.cr3] ;get task CR3
;     cmp eax,edx ;compare current CR3 and task CR3
;     je .skipCR3switch ;skip CR3 if not different - avoid TLB flush
;     mov cr3,edx

;     .skipCR3switch:

;     ; restore segment registers
;     mov ax,[esi + TCB.ds]
;     mov ds,ax
;     mov ax,[esi + TCB.es]
;     mov es,ax
;     mov ax,[esi + TCB.fs]
;     mov fs,ax
;     mov ax,[esi + TCB.gs]
;     mov gs,ax

;     mov eax,[esi + TCB.eip] ;restore EIP
;     mov [esp],eax
;     mov ax,[esi + TCB.cs] ;restore CS
;     mov [esp+4],ax
;     mov eax,[esi + TCB.eflags] ;restore flags
;     mov [esp+8],eax

;     ; restore registers
;     mov eax,[esi + TCB.eax]
;     mov ebx,[esi + TCB.ebx]
;     mov ecx,[esi + TCB.ecx]
;     mov edx,[esi + TCB.edx]
;     mov ebp,[esi + TCB.ebp]
;     mov edi,[esi + TCB.edi]
;     mov esi,[esi + TCB.esi]

;     ret



; struc TCB
;     .eip resd 1
;     .cr3 resd 1
;     .esp resd 1
;     .eax resd 1
;     .ebx resd 1
;     .ecx resd 1
;     .edx resd 1
;     .ebp resd 1
;     .esi resd 1
;     .edi resd 1

;     .ss resw 1
;     .cs resw 1
;     .ds resw 1
;     .es resw 1
;     .fs resw 1
;     .gs resw 1

;     .eflags resd 1
; endstruc