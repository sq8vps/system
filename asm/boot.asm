;---------------------------------------
; 1st stage bootloader located in MBR
; for BIOS-compatiable systems.
; Basically it sets up some stuff, locates the 1,5 stage bootloader in MBR gap
; and loads it.
;---------------------------------------


[bits 16]
[org 0x7c00]
_start:
mov [BOOT_DRIVE],dl ;store boot drive number

cli
mov ax,0x0
mov ds,ax
mov es,ax
mov fs,ax
mov gs,ax

;set stack to 0x7FFFF, there should be quite much space there
mov ax,0x7000
mov ss,ax
mov sp,0xFFFF

mov bx,BOOTLOADER_OFFSET ;destination address
mov edx,BOOTLOADER_SIZE ;bootloader size in sectors
mov ax,0
mov es,ax
mov al,[BOOT_DRIVE]
mov ecx,1 ;start from LBA1, that is the MBR gap
call DiskLoad

mov ax,[BOOTLOADER_OFFSET + 2] ;check for magic number
cmp ax,BOOTLOADER_MAGIC
jne .noLoaderError

xor ax,ax
mov al,[BOOT_DRIVE] ;store drive number in AL for 1,5 stage bootloader

jmp BOOTLOADER_OFFSET ;jump to the bootloader

cli
hlt
jmp $

.noLoaderError:

mov si,noLoaderErrorMsg
call Print16

cli
hlt
jmp $


%include "asm/print16.asm"
%include "asm/disk.asm"
%include "asm/common.asm"


noLoaderErrorMsg db "Boot failed. Bootloader not found.",0xd,0xa,0
times 439-($-$$) db 0 ;fill with zeros
BOOT_DRIVE db 0

