;------------------------------------------------
;2nd stage EAEOS bootloader
;Reads 3rd stage bootloader from FAT32 parition using BIOS (real mode),
;detects available memory regions, sets up flat GDT, A20 line,
;starts protected mode and jumps to the 3rd stage (32-bit) bootloader
;
; IMPORTANT NOTES:
; 1. AL is expeteced to contain BIOS boot drive number (at the beginning).
; 2. MBR partition table must reside at 0x7C00+446 (MBR sector loaded by BIOS)
; 3. 3rd stage bootloader expects memory region entries to reside at MEMDET_DATA
;------------------------------------------------

[bits 16]
[org 0x7E00]

_loaderEntry: jmp _loaderStart ;to musi lezec na poczatku pliku. Robimy skok do wlasciwego wejscia do loadera

db 0x9E ;to musi miec nasz loader, zeby byl rozpoznany. Takie magiczne wartosci
db 0x5F

_loaderStart:
	mov [BOOT_DRIVE],al ;save boot drive number

	mov al,[0x7c00+440] ;store disk signature for bootloader
	mov [DISK_SIG],al
	mov al,[0x7c00+441]
	mov [DISK_SIG+1],al
	mov al,[0x7c00+442]
	mov [DISK_SIG+2],al
	mov al,[0x7c00+443]
	mov [DISK_SIG+3],al

	mov ax,0
	mov es,ax
	mov di,MEMDET_DATA+2 ;at this part we should have almost 30KiB of free memory, so store the memory map there. The first byte will contain number of entries.
	call _detectMemory ;detect available memory regions and save it to 0x501 (as said above)
	mov [MEMDET_DATA],bp ;store number of entries

	;w sumie to jeszcze w 0x7c00 mamy zaladowane MBR, a tam powinna byc tabela partycji, wiec z niej skorzystamy
	mov bl,0 ;zaczynamy od partycji 0
	bootPartLoop:
		cmp bl,4 ;jesli juz 5 wpis, to blad
		je loaderNoBoot
		mov al,16 ;wielkosc wpisu partycji = 16
		mul bl ;mnozymy numer partycji razy wielkosc wpisu (ax=al*ah)
		mov si,446+0x7c00 ;pierwszy wpis 446 bajtow od poczatku MBR + 0x7c00 od poczatku danych
		add si,ax
		inc bl ;zwiekszamy licznik partycji
		mov ah,[si]
		cmp ah,0x80 ;sprawdzamy czy bootowalna
		jne bootPartLoop ;jesli nie, zapetlamy

	add si,4 ;jeszcze typ partycji
	mov ah,[si]
	cmp ah,0xC ;;0xC, czyli FAT32 z LBA
	jne loaderNoBoot

	add si,4 ;spradzamy pierwsze LBA dysku
	xor ecx,ecx
	mov ecx,DWORD[si] ;i pakujemy je do ecx

	mov al,[BOOT_DRIVE]
	call _fatInit ;inicjalizacja woluminu
	cmp dh,0
	jne loaderNoBoot

	mov si,path
	call _fatChangeDir
	cmp dh,0
	jne loaderNoBoot

	mov si,file
	mov ax,0
	mov es,ax
	mov di,LDR32_OFFSET
	call _fatReadFile
	cmp dh,0
	jne loaderNoBoot


	cli
	lgdt [gdt_descriptor] ;ladujemy GDT

	call _enableA20 ;uruchamiamy linie A20

	mov eax,cr0 ;uruchamiamy tryb chroniony
	or eax,1
	mov cr0,eax

	mov ax,DATA_SEG ;data segment selector
	mov ds,ax
	mov ss,ax
	mov es,ax
	mov fs,ax
	mov gs,ax
	mov esp,0x7FFFF


	jmp CODE_SEG:LDR32_OFFSET ;run the 32-bit 3rd stage loader we have read from the disk


	

loaderNoBoot:
	mov si,noSystemMsg
	call _print16
	cli
	hlt

%include "asm/print16.asm"
%include "asm/disk.asm"
%include "asm/fat.asm"
%include "asm/gdt.asm"
%include "asm/a20.asm"
%include "asm/memDet.asm"

BOOT_DRIVE db 0
cpu64bit db 0 ;czy procesor jest 64-bitowy


noSystemMsg db "Operating system not found!",0xD,0xA,"Cannot continue.",0xd,0xa,0,0
path db "/SYSTEM/",0
file db "LDR32",0
LDR32_OFFSET equ 0xA000
LDR_DATA equ 0x500 ;additional data for the 3rd stage bootloader
DISK_SIG equ 0x500 ;disk signature (4 bytes)
MEMDET_DATA equ LDR_DATA + 64 ;memory map for the 3rd stage bootloader

