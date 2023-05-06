;------------------------------------------
;Odczytuje dane z dysku uzywajac BIOSu
;i adresacji LBA (w trybie rzeczywistym)
;(C) Piotr Wilkon 2020
;------------------------------------------

[bits 16]

;laduje sektory z dysku uzywajac LBA
;edx - liczba sektorow do odczytania
;al - numer dysku
;es:bx - adres do ktorego zapisujemy dane. Musi byc wyrownany do 512 bajtow.
;ecx - adres pierwszego sektora do odczytania
;ilosc i poczatkowy adres ograniczony do 32 bitow
_disk_load:		
	push edx
	push ax
	push es
	push ebx
	push ecx
	pushf
	
	test bx,0b111111111 ;jesli adres nie jest wielokrotnoscia 512 bajtow, to blad
	jnz notAlignedErr
	
	mov [diskNumber],al ;zapisujemy numer dysku

	disk_loop:
	
	mov ax,1 ;tylko jeden sektor na raz
	mov [lbaSectors],ax
	mov [lbaDestSeg],es
	mov [lbaDestAddr],bx
	xor ax,ax
	mov [lbaAddrHi],ax
	mov [lbaAddrLo],ecx
	
	mov si,lbaPacket
	
	push edx ;wrzucamy na stos liczbe pozostalych sektorow	

	mov dl,[diskNumber] ;w dl ma byc numer dysku
	mov ah,0x42 ;czytamy sektor

	int 0x13

	pop edx ;i sciagamy liczbe pozostalych sektorow
	
	jc readErr ;jesli blad odczytu
	
	;mov si,progressMsg
	;call _print16

	add bx,512 ;zwiekszamy adres o wielkosc sektora
	
	jnc disk_cont ;jesli nie bylo przejscia przez 0, to kontynuacja
	;jak bylo, to zwiekszamy segment
	mov ax,es
	add ax,0x1000 ;segment to A * 0x10, czyli tutaj idziemy o segment wyzej, ktory sie nie naklada
	mov es,ax
	xor bx,bx
	
	disk_cont:


	inc ecx ;kolejny sektor
	dec edx ;coraz mniej sektorow do odczytania
	jnz disk_loop  ;jesli nie zostalo zero, to zapetlamy   
	
	;mov si,newLineMsg
	;call _print16

	popf
	pop ecx
	pop ebx
	pop es
	pop ax
	pop edx
	
	ret
	
readErr:
	mov si,diskErrMsg
	call _print16
	cli
	hlt
	
notAlignedErr:
	mov si,notAlignedErrMsg
	call _print16
	cli
	hlt

lbaPacket:
	db 0x10 
	db 0
	lbaSectors: dw 0
	lbaDestAddr: dw 0
	lbaDestSeg: dw 0
	lbaAddrLo dd 0
	lbaAddrHi dd 0
	
diskNumber db 0 ;numer dysku
diskErrMsg db 0xd,0xa,"Error: disk cannot be read",0xd,0xa,0
notAlignedErrMsg db 0xd,0xa,"Disk error: destination address is not 512-byte aligned",0xd,0xa,0
progressMsg db ".",0
newLineMsg db 0xd,0xa,0
