[bits 32]

section .text

global _start:function
_start:
	push .1
	mov eax,2 ;file open
	mov ebx,path ;file path
	mov edx,0 ;open mode = 0 = read
	mov esi,0 ;flags = 0
	mov ecx,esp
	sysenter
	.1:
	add esp,4

	mov ebx,eax ;store file handle

	push .2
	mov eax,4 ;file read
	;file handle is in ebx
	mov edx,buffer ;buffer pointer
	mov esi,32 ;size
	mov edi,0 ;offset ls dword
	mov ebp,0 ;offset ms dword
	mov ecx,esp
	sysenter
	.2:
	add esp,4

	jmp $

section .data
path db "/main/system/plik.txt",0

section .bss
buffer resb 32