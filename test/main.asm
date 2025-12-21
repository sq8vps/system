[bits 32]

section .text

global _start:function
_start:
	push .1
	mov eax,5 ;file write
	mov ebx,1 ;file handle - stdout
	mov edx,hello ;buffer pointer
	mov esi,15 ;size
	mov edi,0 ;offset ls dword
	mov ebp,0 ;offset ms dword
	mov ecx,esp
	sysenter
	.1:
	add esp,4

	; push .2
	; mov eax,4 ;file read
	; mov ebx,0 ;file handle - stdin
	; mov edx,buffer ;buffer pointer
	; mov esi,32 ;size
	; mov edi,0 ;offset ls dword
	; mov ebp,0 ;offset ms dword
	; mov ecx,esp
	; sysenter
	; .2:
	; add esp,4

	jmp $

section .data
hello db "Test z programu",0

section .bss
buffer resb 32