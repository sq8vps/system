[bits 32]

%define MULTIBOOT2_EAX_MAGIC 0x36d76289
%define MULTIBOOT2_BUFFER_SIZE 4096 ;this must be synchronized with multiboot.c
;multiboot.c: uint8_t Multiboot2InfoBuffer[MULTIBOOT2_BUFFER_SIZE] ALIGN(8)
extern Multiboot2InfoBuffer

;main.c: NORETURN void KeEntry(struct Multiboot2InfoHeader *mb2h)
extern KeEntry

extern _istart ;linker-defined full image start address
extern _kstart ;linker-defined kernel image start
extern _kend ;linker-defined kernel image end
extern _KERNEL_INITIAL_STACK_ADDRESS ;linker-defined temporary stack

%define PAGE_SIZE 4096
%define PAGE_DIRECTORY_ENTRY_COUNT 1024
%define PAGE_TABLE_ENTRY_COUNT 1024
%define KERNEL_IMAGE_START 0xD6000000
%define KERNEL_IMAGE_END 0xD8000000
;calculate number of pages required for kernel image
;assume both addresses are at the 4-MiB boundary
%define KERNEL_IMAGE_PAGE_COUNT ((KERNEL_IMAGE_END - KERNEL_IMAGE_START) / PAGE_SIZE)

section .bss
align PAGE_SIZE

kernelPageTable:
    ;reserve memory for kernel page tables
    resd KERNEL_IMAGE_PAGE_COUNT
bootPageTable:
    ;reserve memory for page table for out boot code, which must be identity mapped
    resd PAGE_TABLE_ENTRY_COUNT * (1)
pageDir: 
    ;reserve memory for page directory
    resd PAGE_DIRECTORY_ENTRY_COUNT

section .boot

;kernel entry point
global _start:function
_start:
    mov edx,0x7C00 ;set up some wild stack. We need only 4 bytes
    mov esp,edx
    mov ecx,[esp - 4] ;store value that is going to be overwritten by our wild stack
    
    ;the kernel is compiled as a position-dependent code
    ;the bootloader enters this _start function with no paging, thus, logical address = physical address
    ;at the same time, the kernel image may bo loaded at any physical address
    ;to access any kernel memory or functions, the offset must be calculated

    ;use a trick to get EIP on x86
    call .getEip ;this pushes EIP of .getEip on stack
    .getEip:
        pop ebp ;then we should have the physical address of .getEip in EBP
    
    mov edx,.getEip ;get logical address of .getEip
    sub ebp,edx ;calculate and store the offset between physical and logical addresses in EBP
    ;adding EBP to any logical address will convert it to the physical address

    mov [esp - 4],ecx ;restore value that was overwritten by our wild stack

    ;for multiboot2, the magic value should be in EAX and the information structure pointer should be in EBX
    xor eax,MULTIBOOT2_EAX_MAGIC ;compare magic value
    jnz .notMultiboot2

    mov eax,cr0 ;get CR0
    ;disable unnecessary stuff that might be enabled,
    ;such as not-write through or cache-disable
    and eax,0x1FFFFFFF
	mov cr0,eax

    mov ecx,[ebx] ;get total_size field from multiboot2 data

    cmp ecx,MULTIBOOT2_BUFFER_SIZE
    jg .infoTooBig

    ;copy multiboot2 info to buffer
    mov ecx,[ebx] ;get total_size from header
    .copy:
        mov al,[ebx + ecx - 1]
        mov [Multiboot2InfoBuffer + ebp + ecx - 1],al
        loop .copy

    ;set up initial paging

    ;clear page directory and boot page table
    mov ecx,PAGE_TABLE_ENTRY_COUNT
    .1:
        xor eax,eax
        mov [pageDir + ebp + (ecx * 4)],eax
        mov [bootPageTable + ebp + (ecx * 4)],eax
        loop .1
    
    
    ;identity map boot code
    lea edx,[_start+ebp] ;obtain physical address of this function
    mov eax,edx
    shr eax,12 ;calculate page index
    and eax,0x3FF ;calculate index within a page table

    and edx,~0xFFF ;calculate frame address
    or edx,(1 << 0) | (1 << 1) ;add "present" and "writable" bits
    mov [bootPageTable + ebp + (eax * 4)],edx ;store entry in page table
    
    ;there is still a frame address in EDX, use it to insert entry to the page directory
    shr edx,22 ;get page directory index
    lea eax,[bootPageTable+ebp] ;obtain physical address of the page table
    ;this should always be page-aligned
    or eax,(1 << 0) | (1 << 1) ;add "present" and "writable" bits
    mov [pageDir + ebp + (edx * 4)],eax ;store entry in page directory

    ;now map kernel pages
    ;insert entries to page tables
    lea esi,[_kend - PAGE_SIZE]
    mov ecx,_kend ;calculate number of kernel pages
    sub ecx,_kstart
    shr ecx,12 ;divide by 4096 (page size)
    .2:
        lea eax,[esi + ebp] ;obtain physical address
        and eax,~0xFFF ;calculate frame address
        or eax,(1 << 0) | (1 << 1) ;add "present" and "writable" bits
        mov [kernelPageTable + ebp + (ecx * 4) - 4],eax ;store entry in page table
        sub esi,PAGE_SIZE
        loop .2

    ;insert kernel page table entries to page directory
    mov esi,_kend
    test esi,((PAGE_SIZE * PAGE_TABLE_ENTRY_COUNT) - 1) ;check if there is a partially occupied page table
    jz .3
    add esi,(PAGE_SIZE * PAGE_TABLE_ENTRY_COUNT) ;if so, then align
    .3:
    and esi,~((PAGE_SIZE * PAGE_TABLE_ENTRY_COUNT) - 1) ;remove remainder

    mov ecx,esi
    sub ecx,_kstart

    shr ecx,10
    sub ecx,PAGE_SIZE

    sub esi,(PAGE_SIZE * PAGE_TABLE_ENTRY_COUNT) 
    .4:
        mov edx,esi ;get logical address
        shr edx,22 ;calculate page directory entry index

        lea eax,[kernelPageTable + ebp + ecx] ;obtain page table address
        ;this should always be page-aligned
        or eax,(1 << 0) | (1 << 1) ;add "present" and "writable" bits

        mov [pageDir + ebp + (edx * 4)],eax ;store page directory entry
        sub esi,(PAGE_SIZE * PAGE_TABLE_ENTRY_COUNT)
        
        test ecx,0xFFFFFFFF
        jz .5
        sub ecx,PAGE_SIZE
        jmp .4
    
    .5:
    
    ;apply "self-referencing page directory trick"
    lea eax,[pageDir + ebp] ;get page directory physical address
    or eax,(1 << 0) | (1 << 1) ;add "present" and "writable" bits
    mov [pageDir + ebp + (PAGE_DIRECTORY_ENTRY_COUNT - 1) * 4],eax ;store entry in the last slot

    ;from now on, we should have the kernel mapped as it should be
    ;and additionally out boot code is identity mapped,
    ;so we can safely enable paging and start kernel

    lea eax,[pageDir + ebp] ;obtain page directory physical address
    mov cr3,eax ;store it in CR3
    mov eax,cr0
    or eax,(1 << 31) ;set "PG" bit
    mov cr0,eax ;hopefully enable paging

    ;now we should be able to set up the stack
    mov esp,_KERNEL_INITIAL_STACK_ADDRESS
    mov ebp,esp

    ;call via register, otherwise a near jump will be generated and will result in page fault
    mov eax,KeEntry
    push Multiboot2InfoBuffer
    call eax
    ;should be unreachable

    hlt
    jmp $

    .infoTooBig:
        lea esi,[infoTooBigMsg + ebp]
        call _bootPrint

        hlt
        jmp $

    .notMultiboot2:
        lea esi,[notMultiboot2Msg + ebp]
        call _bootPrint

        hlt
        jmp $


;print NULL-terminated string passed in ESI
;mind that ESI needs to contain the physical address before enabling paging
_bootPrint:
    xor ecx,ecx
    .loop:
        mov al, [esi + ecx]
        test al,0xFF
        jz .exit
        mov [0xB8000 + ecx * 2], al
        mov byte [0xB8000 + ecx * 2 + 1], 0x0F
        inc ecx
        jmp .loop

    .exit:
        ret

infoTooBigMsg: db 'Boot failed: bootloader information structure too big',0
notMultiboot2Msg: db 'Boot failed: unknown bootloader',0