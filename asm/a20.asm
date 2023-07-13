[bits 16]

;uruchamia linie A20
_enableA20:
pushf
pusha

call checkA20enabled ;spradzmy, czy juz przypadkiem nie uruchomiona
or al,al
je enableA20end

mov ax,0x2403 ;probujemy uruchomic biosem
int 0x15
jb enableA20biosCont
cmp ah,0
jnz enableA20biosCont

mov ax,0x2402
int 0x15
jb enableA20biosCont
cmp ah,0
jnz enableA20biosCont

cmp al,1
jz enableA20end

mov ax,0x2401
int 0x15
jb enableA20biosCont
cmp ah,0
jnz enableA20biosCont

call checkA20enabled ;spradzmy, czy uruchomiona
or al,al
je enableA20end


enableA20biosCont: ;jak nie dzialalo BIOSem, to kontrolerem klawiatury

call a20wait
mov al,0xad
out 0x64,al

call a20wait
mov al,0xd0
out 0x64,al

call a20wait2
in al,0x60
push eax

call a20wait
mov al,0xd1
out 0x64,al

call a20wait
pop eax
or al,2
out 0x60,al

call a20wait
mov al,0xae
out 0x64,al

call a20wait
call checkA20enabled ;spradzmy, czy uruchomiona
or al,al
je enableA20end

;jak nie, to jeszcze fast a20
in al,0x92
test al,2
jnz enableA20fastCont
or al,2
and al,0xfe
out 0x92,al
enableA20fastCont:

call checkA20enabled ;spradzmy, czy uruchomiona
or al,al
je enableA20end

;jak nie jest nadal uruchomiona, to poddajemy sie
cli
hlt


enableA20end:
popa
popf
ret



a20wait:
in al,0x64
test al,2
jnz a20wait
ret

a20wait2:
in al,0x64
test al,1
jz a20wait2
ret



;sprawdza, czy A20 wlaczone
;jesli tak, to w AL 0. Jesli nie, to 1
checkA20enabled:

pushf
push ds
push es
push di
push si
cli

xor ax,ax ;0
mov es,ax ;0

not ax ;0xFFFF
mov ds,ax ;0xFFFF

mov di,0x0500
mov si,0x0510

mov al,byte[es:di]
push ax

mov al,byte[ds:si]
push ax

mov byte[es:di],0x00
mov byte[ds:si],0xFF

cmp byte[es:di],0xFF

pop ax
mov byte[ds:si],al

pop ax
mov byte[es:di],al

mov ax,1
je checkA20exit

mov ax,0

checkA20exit:
sti
pop si
pop di
pop es
pop ds
popf
ret
