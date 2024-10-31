[bits 32]

global _start:function
_start:
	push .1
	mov eax,12345
	mov ecx,esp
	sysenter
	.1:
	add esp,4
	jmp $
