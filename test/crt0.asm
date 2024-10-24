[bits 32]

extern main
;extern __x86.get_pc_thunk.ax

section .text
global _start:function
_start:
	mov ebp,esp
	
	;call __x86.get_pc_thunk.ax
	;add eax,main - $
	cld
	;call eax
	call main
	
	jmp $
