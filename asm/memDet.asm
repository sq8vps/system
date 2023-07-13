[bits 16]

;ES:DI - memory map entries destination buffer
;returns BP = entry count
_detectMemory:

	pushf
	push es
	push di

	xor ebx,ebx
	xor bp,bp
	mov edx,0x0534D4150 ;magic number required by bios
	mov eax,0xe820 ;command id
	;returned entries can be 20 or 24 bytes long. We always keep them as 24 bytes for uint64_t alignment
	;24 byte format is the ACPI 3.x format and requires bit 0 to be set in a byte with offset +20 to indicate that this entry should NOT be ignored
	mov [es:di + 20],dword 1
	mov ecx,24 ;read 24 bytes
	int 0x15 ;issue command

	jc .dmFail ;if carry, the command is not supported
	mov edx,0x0534D4150 ;some bioses can clear this register, so fill it with magic number
	cmp eax,edx ;if command is successful, eax should contain magic number
	jne .dmFail
	test ebx,ebx ;if ebx=0, this is the end of the list, so in this point it means there is only one entry,  which is worthless
	je .dmFail
	jmp .dmCont


.dmNext:
	test ebx,ebx
	jz .dmSort ;if ebx=0, there is no more entries
	;else continue
	mov eax,0xe820 ;command id
	mov [es:di + 20],dword 1 ;explained above
	mov ecx,24 ;24 bytes to read
	int 0x15 ;issue command
	jc .dmSort ;if carry bit is set, the end of the list has been reached
	mov edx,0x0534D4150 ;some bioses can clear this register, so fill it with magic number
	cmp eax,edx ;if command is successful, eax should contain magic number
	jne .dmFail

.dmCont:
	jcxz .dmNext ;if cx=0, the enry is 0 length
	cmp cl,20 ;check entry length
	je .dmRead ;if it's 20 bytes long process entry
	;if it's a 24-byte entry
	test byte[es:di + 20],1 ;check if there is "ignore this entry" bit clear
	je .dmNext ;if so, drop this entry
	;if not, continue to .dmRead
.dmRead:
	mov eax,[es:di + 16] ;check type field
	cmp eax,1 ;we are interested only in OS-usable (type=1) memory regions
	jne .dmNext ;if it's reserved, skip it
	mov ecx,[es:di + 8] ;get lower dword of region length
	or ecx,[es:di + 12] ;or the lower dword with higher dword. If both are 0, this will result in 0
	jz .dmNext ;regions with 0 length are useless
	inc bp ;increment entry counter
	add di,24 ;set buffer pointer
	jmp .dmNext

.dmSort:
	push di
	mov si,100 ;arbitrary initial value for the swap counter
.dmSortMain: ;sort the entries. They are usually already sorted, but there is no guarantee
	;use bubble sort
	or si,si
	jz .dmSortEnd
	xor si,si ;reset swap counter
	mov cx,1
	pop di
	push di
.dmSortLoop:
	mov eax,[es:di + 4]
	mov ebx,[es:di + 28]
	cmp eax,ebx ;compare higher address dwords
	je .dmSortHiEq ;if they are equal, we need to check lower byte
	ja .dmSortSwap ;if high dword of the first address is bigger than the next, we need to swap them
	;if the first one is smaller, the order is OK
	jmp .dmSortCont

.dmSortHiEq: ;if high address entries are equal
	mov eax,[es:di]
	mov ebx,[es:di + 24]
	cmp eax,ebx ;compare lower address dwords
	jna .dmSortCont ;if are equal or the first one is smaller, everything is OK
	;if the first one is higher, swap them

.dmSortSwap: ;swapping two consecutive entries
	inc si ;increase swap counter
	xor cl,cl ;counter
	push di ;save initial index
	.dmSortSwapLoop:
		mov edx,[es:di] ;store first entry
		mov eax,[es:di+24] ;replace first entry with a second one
		mov [es:di],eax
		mov [es:di+24],edx ;and the second one with the first one
		add di,4
		inc cl
		cmp cl,6
		jb .dmSortSwapLoop
	xor cl,cl
	pop di


.dmSortCont:
	add di,24 ;jump to the next entry
	inc cx
	cmp cx,bp ;check if we've reached the last entry - if so, we need to start over
	je .dmSortMain
	jmp .dmSortLoop


.dmSortEnd:
	pop di
	pop di
	pop es
	popf
	ret

.dmFail:
	mov si,dmError
	call _print16

	cli
	hlt







;finds the lowest memory region in the extended memory of a specified size or bigger
;ES:DI - memory map buffer. This function assumes the map is already sorted.
;BP - number of entries
;EDX - minimum size in bytes (EDX is 32-bit, so at most 4 GiB)
;returns CX - entry number (1 - first entry, etc., 0 - no matching entry found)
_findMemoryRegion:
	pushf
	push es
	push di
	push bp
	push eax
	push edx

	xor ecx,ecx
	mov cx,1 ;loop counter

	.findRegLoop:
	;again, this function assumes memory map to be already sorted
		mov eax,[es:di + 4] ;check higher address dword
		jnz .findRegSize ;if different than 0, this region is definitely in the extended memory part
		;if zero
		mov eax,[es:di] ;check lower address dword
		cmp eax,0x100000 ;check if this address is in the extended memory (higher than/equal to 0x100000)
		jb .findRegCont ;if it's in the real mode memory part, skip it

		.findRegSize: ;if it's in the extended memory, check the size
		mov eax,[es:di+12] ;check higher size dword
		jnz .foundReg ;if different than 0, the size of this region is at least 4 GiB, so that's the region we are looking for
		;if zero
		mov eax,[es:di+8] ;check lower size dword
		cmp eax,edx ;compare with the required size
		jb .findRegCont ;if smaller, skip it
		jmp .foundReg;else that's the region we are looking for

	.findRegCont:
		add di,24 ;next entry
		inc cx ;increse loop counter
		cmp cx,bp
		jbe .findRegLoop ;if there are still some entries
		;if not, the region was not found
		xor cx,cx ;set return value to 0 (not found)

.foundReg:

	pop edx
	pop eax
	pop bp
	pop di
	pop es
	popf
	ret



;shows available memory regions as addrLo addrHi sizeLo sizeHi<cr><lf>addrLo addrHi ...
;ES:SI - memory map buffer
;BP - number of entries
_showDetectedMemory:
	pushf
	push bp
	push edx
	push es
	push di

.pDMloop:
	mov edx,[es:di]
	call _print16hex
	mov si,dmSpace
	call _print16
	mov edx,[es:di+4]
	call _print16hex
	mov si,dmSpace
	call _print16
	mov edx,[es:di+8]
	call _print16hex
	mov si,dmSpace
	call _print16
	mov edx,[es:di+12]
	call _print16hex
	mov ax,0
	mov ds,ax
	mov si,dmNewLine
	call _print16

	add di,24
	dec bp
	or bp,bp
	jnz .pDMloop

pop di
pop es
pop edx
pop bp
popf
ret


dmNewLine db 0x0d,0x0a,0
dmSpace db ' ',0


dmError db "Memory detection failed.",0x0d,0x0a,0

dmTempEntry times 6 dd 0

