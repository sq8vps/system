[bits 16]
;laduje DH sektorow do ES:BX z dysku DL
disk_load:	
	push dx ;zapisujemy ile sektorow odczytac
	
	mov ah,0x02 ;odczytujemy z dysku po CHS
				;TODO: przerobic to na LBA
	mov al,dh
	mov ch,0 ;cylinder 0
	mov dh,0 ;glowica 0
	mov cl,0x02 ;od drugiego sektoru
	int 0x13 
	
	jc blad ;jesli blad odczytu
	
	pop dx ;sprawdzamy jaka liczba sektorow miala byc odczyta
	cmp dh,al ;jesli przewidywana jest rozna od odczytanej to blad
	jne blad
	ret
	
blad:
	mov si,err ;adres lanucha tekstowego
	mov ah,0xe ;wyjscie tekstu na ekran przez bios
petlad:
	mov al,[si] ; do al wpychamy bajt z tekstu (wartosc)
	inc si ;zwieksamy wskaznik na kolejny element tekstu
	cmp al,0 ;robimy petle az trafimy na NUL na koncu stringa
	je $ ;nieskonczona petla gdy koniec tekstu
	int 0x10 ;przerwanie biosu
	jmp petlad


err db "Disk read failed",0
