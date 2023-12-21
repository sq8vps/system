[bits 32]

;drukuje string zakonczony nulem z adresem w EBX
print32:
pushfd
pushad
mov edx,0xb8000 ;tu jest pamiec VGA

print32loop:
mov al,[ebx] ;zapisujemy kolejny znak
mov ah,0x0f ;atrybuty tekstu

cmp al,0 ;wykonujemy az do NULa
je print32done

mov [edx],ax ;zapisujemy do pamieci karty znak z atrybutami
inc ebx ;zwiekszamy wskaznik na znak
add edx,2 ;zwiekszamy wskaznik na kolejne komorke pamieci VGA

jmp print32loop

print32done:
popad
popfd
ret
