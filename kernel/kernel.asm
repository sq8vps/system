[bits 32]

_kernelEntry:
	mov al,'U'
	out 0xe9,al
kernelLoop: jmp kernelLoop
