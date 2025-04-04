[bits 16]
[org 0x7C00]

xor ax,ax
mov es,ax

mov ax,0x7000
mov ss,ax
mov sp,0xFFF0

mov di,0x7E00
mov [di],DWORD 'VBE2'

mov ax,0x4F00
int 0x10

jmp $

times 510-($-$$) db 0
dw 0xAA55
