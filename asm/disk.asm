;------------------------------------------
;Read disk using BIOS calls and LBA addressing
;------------------------------------------

[bits 16]

;load sectors using BIOS with LBA addressing
;edx - count of sectors
;al - disk number
;es:bx - destination address. Must be 512B aligned.
;ecx - first LBA
DiskLoad:		
	push edx
	push ax
	push es
	push ebx
	push ecx
	pushf
	
	test bx,0b111111111 ;check alignment
	jnz .notAlignedErr
	
	mov [diskNumber],al ;store disk number

	.diskLoop:
	
	mov ax,1 ;read 1 sector
	mov [lbaSectors],ax
	mov [lbaDestSeg],es
	mov [lbaDestAddr],bx
	xor ax,ax
	mov [lbaAddrHi],ax
	mov [lbaAddrLo],ecx
	
	mov si,lbaPacket ;store address of LBA packet
	
	push edx ;store remaining sectors	

	mov dl,[diskNumber] ;get disk number
	mov ah,0x42 ;sector read command

	sti
	int 0x13
	cli

	pop edx ;restore remaining sectors
	
	jc .readErr

	add bx,512 ; increment destination address by sector size
	
	jnc .diskCont ;continue if no segment change
	;else update segment
	mov ax,es
	add ax,0x1000
	mov es,ax
	xor bx,bx
	
	.diskCont:

	inc ecx ; next sector to be read
	dec edx ; decrement number of remaining sectors
	jnz .diskLoop ; continue if there are remaining sectors

	popf
	pop ecx
	pop ebx
	pop es
	pop ax
	pop edx
	
	ret
	
.readErr:
	mov si,diskErrMsg
	call Print16
	cli
	hlt
	jmp $
	
.notAlignedErr:
	mov si,notAlignedErrMsg
	call Print16
	cli
	hlt
	jmp $

lbaPacket:
	db 0x10 
	db 0
	lbaSectors: dw 0
	lbaDestAddr: dw 0
	lbaDestSeg: dw 0
	lbaAddrLo dd 0
	lbaAddrHi dd 0
	
diskNumber db 0
diskErrMsg db 0xd,0xa,"Disk read failed",0xd,0xa,0
notAlignedErrMsg db 0xd,0xa,"Disk read failed: 512-byte alignment required",0xd,0xa,0
