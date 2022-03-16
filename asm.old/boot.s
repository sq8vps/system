[bits 16] ;uzywamy 16 bitow
[org 0x7c00] ;tam bedzie program, potrzebne dla prawidlowego liczenia adresow

[global _boot]

LOADER_OFFSET equ 0x1000 ;przesuniecie adresu dla ladowania drugiego loadera

_boot:

mov [BOOT_DRIVE],dl ;zapisujemy numer dysku rozruchowego

mov bp,0x9000 ;zmienimy wskaznik stosu, zeby przypadkiem nie uszkodzic stosu
mov sp,bp

mov bx,LOADER_OFFSET ;odczytujemy loader z dysku
mov dh,9
mov dl,[BOOT_DRIVE]
call disk_load

call switch_pm
	
jmp $
	
%include "asm/gdt.s"
%include "asm/switch_pm.s"
%include "asm/print32.s"
%include "asm/disk.s"

[bits 32]

BEGIN_PM: ;tutaj jestesmy na powaznie w trybie chronionym

;mov ebx,msg
;call print32


call LOADER_OFFSET

jmp $

BOOT_DRIVE db 0


msg db "Jestemy w trybie chronionym",0
times 510-($-$$) db 0 ;wypelniamy reszte pliku 0
dw 0xaa55 ;oznaczneie bootowalnego dysku
