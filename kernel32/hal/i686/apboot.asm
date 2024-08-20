[bits 16]
; WARNING! This code MUST be loaded at physical address 0x1000


; a mixed 16/32-bit CPU boostrap routine
; it is called by the main CPU using inter-processor-interrupt
global I686StartAp:function
I686StartAp:
    ;disable interrupts, clear direction flag and continue execution
    cli
    cld
    jmp 0:0x1040

;set up initial GDT, use fixed 16-bit addresses
    align 16
    ;0x1010
    gdt_start:
    ;null descriptor
    dd 0x0
    dd 0x0 
    gdt_code: ;code descriptor
    dw 0xffff ;limit
    dw 0x0 ;base
    db 0x0 ;base
    db 10011010b ;flags
    db 11001111b ;flags + limit
    db 0x0 ;base 
    gdt_data: ;data descriptor
    dw 0xffff ;limit
    dw 0x0 ;base
    db 0x0 ;base
    db 10010010b ;flags
    db 11001111b ;flags + limit
    db 0x0 ;base
    gdt_end:
    align 16
    gdt_descriptor equ 0x1030
    ;0x1030
    dw gdt_end - gdt_start - 1 ;GDT size
    dd 0x1010 ;GDT address

    CODE_SEG equ gdt_code - gdt_start
    DATA_SEG equ gdt_data - gdt_start

    align 64
; 0x1040 - continue execution:
    xor ax,ax ;clear data segment register
    mov ds,ax
    lgdt [gdt_descriptor] ;load GDT

    mov eax,cr0 ;enable protected mode - bit 0 of CR0
    or eax,1
    mov cr0,eax

    jmp CODE_SEG:0x1060 ;far jump to 32-bit code to start 32-bit mode

[bits 32]
    align 32
    ;0x1060

    mov ax,DATA_SEG
    mov ds,ax
    mov ss,ax
    mov es,ax
    mov fs,ax
    mov gs,ax

    ;get LAPIC ID from CPUID, EAX=1, bits 24...31 in EBX
    mov eax,1
    cpuid
    shr ebx,24

    mov esi,0x2000 ;store bootstrap table pointer
.findId:
    mov edx,[esi + I686BootstrapData.esp] ;get stack pointer
    and edx,edx ;check if zero
    ;if so, we've reached the end of the table, which is not good
    jz .notFound

    mov edx,[esi + I686BootstrapData.lapic] ;get LAPIC ID from table
    xor edx,ebx ;compare IDs, 0 after XORing if both are the same
    jz .found
    ;not matching, continue
    add esi,20
    jmp .findId

.found:
    mov esp,[esi + I686BootstrapData.esp] ;store stack pointer
    mov edi,[esi + I686BootstrapData.eip] ;store bootstrap routine pointer
    mov eax,[esi + I686BootstrapData.cr3] ;update CR3 with page directory
    mov edx,[esi + I686BootstrapData.id] ;store kernel CPU number
    mov cr3,eax
	mov eax,cr0 ;get CR0
    ;disable unnecessary stuff that might be enabled,
    ;such as not-write through or cache-disable
    and eax,0x1FFFFFFF
    ;enable paging
	or eax,0x80000000
	mov cr0,eax

    ;call C bootstrap routine with CPU number as a parameter
    cld
    push edx
    call edi

.notFound:
    hlt
    jmp .notFound

global I686StartApEnd:function
I686StartApEnd:


struc I686BootstrapData
    .id resd 1
    .lapic resd 1
    .cr3 resd 1
    .esp resd 1
    .eip resd 1
endstruc