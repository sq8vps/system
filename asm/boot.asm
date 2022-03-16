;---------------------------------------
;To jest najprostszy bootloader do MBR dysku
;W zasadzie laduje tylko nastepny stopien bootloadera
;z pierwszego sektora dysku
;(C) Piotr Wilkon 2020
;---------------------------------------


[bits 16]
[org 0x7c00]
_start:

mov [BOOT_DRIVE],dl ;zapisujemy numer dysku rozruchowego, z ktorego uruchiomiony byl bootloader

cli
;rejestry segmentowe na prawidlowy adres
mov ax,0x0
mov ds,ax
mov es,ax
mov fs,ax
mov gs,ax

;ustawiamy stos na 7FFFF
mov ax,0x7000
mov ss,ax
mov sp,0xFFFF

mov bx,LOADER_OFFSET ;odczytujemy loader z dysku do tego adresu
mov edx,LOADER_SIZE ;jego wielkosc
mov ax,0
mov es,ax
mov al,[BOOT_DRIVE]
mov ecx,1
call _disk_load

;sprawdzamy czy jest prawidlowy bootloader, ktory oznaczaja magiczne wartosci 0x9e, 0x5f
mov ax,[LOADER_OFFSET + 2]
cmp ax,0x5f9e
jne noLoaderError

xor ax,ax
mov al,[BOOT_DRIVE] ;!!!! keep drive number in al

jmp LOADER_OFFSET ;idziemy do loadera

cli
hlt

noLoaderError:

mov si,noLoaderErrorMsg
call _print16
cli
hlt


%include "asm/print16.asm"
%include "asm/disk.asm"

LOADER_OFFSET equ 0x7E00
LOADER_SIZE equ 4096


noLoaderErrorMsg db "Error: EAEOS bootloader not found!",0xd,0xa,"Cannot continue.",0xd,0xa,0
times 439-($-$$) db 0 ;wypelniamy reszte pliku 0
BOOT_DRIVE db 0

