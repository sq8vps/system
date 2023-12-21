;------------------------------------
;Print text in real mode
;------------------------------------

[bits 16]

;Print string from DS:SI terminated with null
Print16:
	push ax

.print16loop:
	lodsb
	or al,al ;chceck if al=0 (null terminator)
	jz .print16done ;if so, stop
	mov ah,0x0e ;command id
	sti
	int 0x10 ;issue command
	cli
	jmp .print16loop
	
.print16done:
	pop ax
	ret

; ;prints EDX value as hex (0xZZZZZZZZ)
; Print16hex:
; 	pushf
; 	push eax
; 	push ebx
; 	push ecx

; 	mov al,'0'
; 	mov ah,0x0e
; 	int 0x10
; 	mov al,'x'
; 	mov ah,0x0e
; 	int 0x10

; 	mov cl,32
; .print16hexloop:
; 	or cl,cl
; 	jz .print16hexEnd
; 	sub cl,4
; 	mov ebx,edx
; 	shr ebx,cl
; 	mov al,bl
; 	and al,0xF
; 	cmp al,9
; 	ja .print16hexLetter
; 	add al,48
; 	mov ah,0x0e
; 	int 0x10
; 	jmp .print16hexloop

; .print16hexLetter:
; 	add al,55
; 	mov ah,0x0e
; 	int 0x10
; 	jmp .print16hexloop

; .print16hexEnd:
; 	pop ecx
; 	pop ebx
; 	pop eax
; 	popf

; 	ret



