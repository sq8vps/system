
[bits 16]

;inicjalizuje modul FAT32
;przyjmuje w AL numer dysku
;a w ECX LBA pierwszego (zerowego) sektora partycji FAT32
;zwraca w DH kod bledu: 0 - powodzenie, 1 - niepowodzenie
_fatInit:
	pushf
	push eax
	push ecx
	push ebx

	mov [fatDiskNumber],al

	mov edx,1 ;czytamy tylko jeden sektor
	xor bx,bx
	mov es,bx
	mov bx,FAT_TEMP ;tymczasowy adres na dane. Powinno tam byc 30KiB wolnego
	call _disk_load ;czytamy naglowek fat

	mov bx,WORD [FAT_TEMP+11] ;czytamy ile jest bajtow na sektor. Akecptujemy tylko 512
	cmp bx,512
	jne fatInitError

	xor ebx,ebx
	mov bl,BYTE [FAT_TEMP+13]
	mov [fatSecPerClus],bl ;sektory na klaster

	imul bx,512
	mov [fatBytesPerClus],bx


	xor ebx,ebx
	mov bx,WORD [FAT_TEMP+14] ;zarezerwowane sektory

	cmp bx,32
	jne fatInitError

	add ecx,ebx ;dodajemy do pierwszego LBA partycji
	mov [fatFATSector],ecx ;i mamy adres pierwszego FATu


	mov ebx,DWORD [FAT_TEMP+36] ;rozmiar FATu
	add ecx,ebx ;dodajemy do lokalizacji 1 FATu
	mov [fatFAT2Sector],ecx ;i mamy 2 FAT

	add ecx,ebx ;dodajemy jeszcze raz
	mov [fatDataSector],ecx ;i mamy pierwszy sektor z danymi

	mov ebx,DWORD [FAT_TEMP+44] ;numer pierwszego klastra z danymi (root directory)
	mov [fatFirstDataCluster],ebx
	mov [fatCurrDirCluster],ebx ;poczatkowy folder to root directory

	xor dh,dh
	jmp fatInitOk

	fatInitError:
		mov dh,1
	fatInitOk:

	pop ebx
	pop ecx
	pop eax
	popf
	ret



;przechodzi do wybranego folderu
;przyjmuje w DS:SI sciezke do folderu, gdzie "/" to root driectory,
;np. "/system/drivers/"
;zwraca w DH kod bledu: 0 - powodzenie, 1 - brak folderu, 2 - niepoprawna sciezka, 3 - dane uszkodzone, 4 - blad odczytu
_fatChangeDir:
	pushf
	push eax
	push ecx
	push ebx

	mov edx,[fatFirstDataCluster] ;na poczatek ustawiamy sie na root directory
	mov [fatCurrDirCluster],edx

	readNextDirPathEl:
	xor di,di
	mov byte[fileNameBuffer],0
	mov edx,[fatCurrDirCluster]

	readDirNameLoop:
	lodsb ;ladujemy znak do al
	cmp al,"/" ;jesli przejscie do kolejnego folderu, to skok
	je findDirSlash
	or al,al ;jesli koniec sciezki, czyli nul
	jz findDir
	cmp di,255 ;a jesli juz jest za duzo znakow
	je changeDirErr2
	mov [fileNameBuffer+di],al ;zapisujemy znak w buforze
	mov byte[fileNameBuffer+di+1],0 ;prostym sposobem zawsze dajemy nul, ktory ewentualnie zostanie potem nadpisany
	inc di
	jmp readDirNameLoop

	findDirSlash:
	or di,di
	jnz findDir ;jesli bylo >0 znakow, kontynuujemy normalnie
	cmp edx,[fatFirstDataCluster] ;a jesli bylo 0, ale to pierwszy element, to jest root directory, wiec mozemy to pominac
	je readNextDirPathEl
	;a jesli bylo 0 znakow, ale to nie pierwszy element, to blad
	jmp changeDirErr2

	findDir: ;tu juz jest rzeczywiste szukanie folderu. W di powinna byc dlugosc nazwy
	mov [pathIdx],si
	or di,di ;jesli tutaj bylo zero znakow, czyli na przyklad ostatni slash w sciezce i potem nic, to to pomijamy
	jz changeDirOk


	mov bp,0 ;folder
	call fatFindFile
	cmp dh,0
	je dirFound
	jmp fatChangeDirExit



	dirFound: ;znaleziono katalog

	xor eax,eax
	mov ax,word[FAT_CLUSTER+bx+0x14] ;czytamy wyzsze slowo adresu poczatkowego klastra dla tego folderu
	shl eax,16
	mov ax,word[FAT_CLUSTER+bx+0x1a]
	cmp eax,1 ;i sprawdzamy, czy wiekszy od 1
	jna changeDirErr3 ;jak nie, to uszkodzone dane
	mov [fatCurrDirCluster],eax ;zapisujemy aktualny katalog

	mov si,[pathIdx]
	jmp readNextDirPathEl ;przetwarzamy kolejny element sciezki


	changeDirOk:

	mov dh,0
	jmp fatChangeDirExit

	changeDirErr1:
	mov dh,1
	jmp fatChangeDirExit
	changeDirErr2:
	mov dh,2
	jmp fatChangeDirExit
	changeDirErr3:
	mov dh,3
	jmp fatChangeDirExit
	changeDirErr4:
	mov dh,4
	jmp fatChangeDirExit

	fatChangeDirExit:
	pop ebx
	pop ecx
	pop eax
	popf
	ret


;odczytuje wybrany plik z dysku FAT32
;przyjmuje w DS:SI nazwe pliku
;przyjmuje w ES:DI adres, pod ktory plik zostanie zapisany
;zwraca w DH kod bledu: 0 - powodzenie, 1 - brak pliku, 3 - dane uszkodzone, 4 - blad odczytu
_fatReadFile:
	pushf
	push eax
	push ecx
	push ebx
	push es
	push di
	push ds
	push esi

	push di
	xor di,di
	checkNameLoop:
	lodsb
	mov [fileNameBuffer+di],al ;zapisujemy nazwe zadanego pliku do zmiennej
	inc di
	or al,al
	jnz checkNameLoop

	mov bp,1 ;typ to plik
	call fatFindFile
	cmp dh,0
	pop di
	jne fatReadFileErr1

	xor ecx,ecx
	mov cx,word[FAT_CLUSTER+bx+0x14] ;czytamy wyzsze slowo adresu poczatkowego klastra
	shl ecx,16
	mov cx,word[FAT_CLUSTER+bx+0x1a]
	cmp ecx,1 ;i sprawdzamy, czy wiekszy od 1
	jna fatReadFileErr3 ;jak nie, to uszkodzone dane

	xor esi,esi

	fatReadFileNextCluster:
	call fatReadCluster ;klaster powinien byc w FAT_CLUSTER
	fatReadFileLoop:
	mov eax,[FAT_CLUSTER+esi] ;przenosimy dwordy
	mov [es:di],eax
	add esi,4
	add di,4 ;zwiekszamy dolna czesc o 4
	jnc fatReadFileLoopCont ;jak wyjechala poza zakres
	mov ax,es ;to gorna tez trzeba zwiekszyc
	add ax,4096
	mov es,ax
	fatReadFileLoopCont: ;a jak nie, to kontynuujemy
	xor eax,eax
	mov ax,[fatBytesPerClus]
	cmp esi,eax
	jb fatReadFileLoop ;jesli jeszcze nie skonczylismy klastra, to dalej przepisujemy
	;a jak skonczylismy, to sprawdzamy, czy jest kolejny
	xor esi,esi
	mov bl,0 ;pierwszy FAT
	mov eax,ecx
	call fatCheckNextCluster
	and edx,0x0FFFFFFF ;gorne 4 bity sa zarezerwowane
	cmp edx,0xFFFFFF8 ;sprawdzamy czy to nie ostatni klaster
	jnb fatReadFileOK ;jak ostatni, to konczymy
	cmp edx,0 ;sprawdzmy przypadki bledow, wtedy korzsytamy z zapasowego FATu
	je readFileCheckAltFAT
	cmp edx,1
	je readFileCheckAltFAT
	cmp edx,0xFFFFFF7
	je readFileCheckAltFAT
	;jesli wszystko bylo dobrze, to mamy nastepny klaster, wiec czytamy dalej
	mov ecx,edx
	jmp fatReadFileNextCluster

	readFileCheckAltFAT: ;sprawdzanie zapasowego FATu
	mov eax,ecx
	mov bl,1 ;zapasowy FAT
	call fatCheckNextCluster ;sprawdzamy nastepny klaster w FAT, powinien byc w EDX
	and edx,0x0FFFFFFF ;gorne 4 bity sa zarezerwowane
	cmp edx,0xFFFFFF8 ;sprawdzamy czy to nie ostatni klaster
	jnb fatReadFileOK ;jak ostatni, to konczymy
	cmp edx,0 ;sprawdzmy przypadki bledow, wtedy nie mozemy kontynuowac
	je fatReadFileErr3
	cmp edx,1
	je fatReadFileErr3
	cmp edx,0xFFFFFF7
	je fatReadFileErr3
	;jesli wszystko bylo dobrze, to mamy nastepny klaster, wiec czytamy dalej
	mov ecx,edx
	jmp fatReadFileNextCluster


	fatReadFileOK:
	mov dh,0
	jmp fatReadFileExit
	fatReadFileErr1:
	mov dh,1
	jmp fatReadFileExit
	fatReadFileErr3:
	mov dh,3
	jmp fatReadFileExit

	fatReadFileExit:
	pop esi
	pop ds
	pop di
	pop es
	pop ebx
	pop ecx
	pop eax
	popf
	ret





;znajduje plik lub folder
;nazwa pliku/folderu w zmiennej fileNameBuffer, jej dlugosc w DI
;numer klastra poczatkowego w fatCurrDirCluster
;w BP: 0 - folder, 1 - plik
;zwraca w DH kod bledu: 0 - powodzenie, 1 - brak pliku/folderu, 2 - niepoprawna sciezka, 3 - dane uszkodzone, 4 - blad odczytu
;zwraca w BX przesuniecie wpisu (od FAT_CLUSTER) katalogu zawierajace zadany element
fatFindFile:

	pushf
	push eax
	push ecx
	push di
	push es

	shl bp,4
	mov [requiredType],bp
	findDirBegin:

	mov ecx,[fatCurrDirCluster] ;odczytujemy klaster
	call fatReadCluster
	or dh,dh
	jnz findDirErr4

	xor bx,bx

	findDirLoop: ;petla, ktora leci po kolejnych wpisach

	xor ax,ax
	mov al,[fatSecPerClus] ;sektory na klaster
	shl ax,9 ;*512 i mamy bajty, ktore mieszcza sie w klastrze
	cmp bx,ax ;sprawdzamy, czy nie skonczyl sie ten klaster
	jb findDirLoopCont ;jak nie, to kontynuujemy
	;jak tak, to musimy sie troche pobawic
	mov eax,edx
	mov bl,0
	call fatCheckNextCluster ;sprawdzamy nastepny klaster w FAT, powinien byc w EDX
	and edx,0x0FFFFFFF ;gorne 4 bity sa zarezerwowane
	cmp edx,0xFFFFFF8 ;sprawdzamy czy to nie ostatni klaster
	jnb findDirErr1 ;jak tak, to nie znaleziono folderu
	cmp edx,0 ;sprawdzmy przypadki bledow, wtedy korzsytamy z zapasowego FATu
	je findDirCheckAltFAT
	cmp edx,1
	je findDirCheckAltFAT
	cmp edx,0xFFFFFF7
	je findDirCheckAltFAT
	;jesli wszystko bylo dobrze, to wracamy na poczatek
	mov [fatCurrDirCluster],edx
	jmp findDirBegin

	findDirCheckAltFAT: ;sprawdzanie zapasowego FATu
	xor ax,ax
	mov al,[fatSecPerClus] ;sektory na klaster
	shl ax,9 ;*512 i mamy bajty, ktore mieszcza sie w klastrze
	cmp bx,ax ;sprawdzamy, czy nie skonczyl sie ten klaster
	jb findDirLoopCont ;jak nie, to kontynuujemy
	mov eax,edx
	mov bl,1 ;zapasowy FAT
	call fatCheckNextCluster ;sprawdzamy nastepny klaster w FAT, powinien byc w EDX
	and edx,0x0FFFFFFF ;gorne 4 bity sa zarezerwowane
	cmp edx,0xFFFFFF8 ;sprawdzamy czy to nie ostatni klaster
	jnb findDirErr1 ;jak tak, to nie znaleziono folderu
	cmp edx,0 ;sprawdzmy przypadki bledow, wtedy nie mozemy kontynuowac
	je findDirErr3
	cmp edx,1
	je findDirErr3
	cmp edx,0xFFFFFF7
	je findDirErr3
	;jesli wszystko bylo dobrze, to wracamy na poczatek
	mov [fatCurrDirCluster],edx
	jmp findDirBegin

	findDirLoopCont:
	mov al,[FAT_CLUSTER+bx] ;odczytujemy zerowy bajt kazdego wpisu
	or al,al ;jesli jakis jest 0, to koniec tablicy
	jz findDirErr1 ;wiec blad

	xor ax,ax
	mov al,[FAT_CLUSTER+bx+0xB]
	cmp al,0x0F
	je findDirContinue
	and ax,0x10
	mov bp,[requiredType]
	and bp,0x10
	xor ax,bp
	jz findDirContinue ;idziemy do nastepnego wpisu

	mov al,[FAT_CLUSTER+bx-21] ;cofamy sie do wpisu wczesniej
	cmp al,0x0F ;i sprawdzmy, czy to wpis LFN (dlugiej nazwy)
	jne findDirParseShortName ;jesli nie, to przetwarzamy krotka nazwe

	;a jesli jest dluga nazwa
	jmp findDirParseLFN

	findDirContinue:
	add bx,32
	jmp findDirLoop

	findDirParseShortName:
	test bp,0xFFFF
	jnz findFileParseShortName ;nazwy plikow obslugujemy inaczej

	cmp di,8 ;jesli poszukiwany jest folder z nazwa dluzsza niz 8 bajtow, to tutaj nic nie zrobimy
	ja findDirContinue
	mov si,di ;musimy sprawdzic wszystkie 11 bajtow. Dolepimy je do nazwy pliku
		shortNameAddSpaces:
			cmp si,11 ;jesli dojdziemy do 11 indeksu, to koniec
			je parseShortNameLoop
			mov byte[fileNameBuffer+si],0x20 ;dolepiamy spacje
			inc si
			jmp shortNameAddSpaces

		parseShortNameLoop:
		dec si ;odejmujemy 1 i wtedy mamy indeks dla zmiennej
		mov al,byte[FAT_CLUSTER+bx+si] ;nazwa na dysku
		mov ah,byte[fileNameBuffer+si] ;nazwa zadana
		cmp ah,al ;jesli sie nie zgadzaja
		jne findDirContinue ;to szukamy dalej
		or si,si ;jesli doszlismy do zera i wszystko sie zgadzalo jak dotad (jesli by sie nie zgadzalo, to jmp wyzej by zadzialal)
		jz found ;to ten folder
		jmp parseShortNameLoop ;zapetlamy

	findFileParseShortName: ;z plikami obchodzimy sie inaczej
	cmp di,12 ;pliki z nazwa ponad 12 bajtow (wlacznie z kropka) obslugujemy gdzie indziej
	ja findDirContinue

	xor si,si
	xor di,di
		convertShortName: ;prosciej bedzie przekonwertowac nazwe z typu fat na czytelna i potem je porownac
		mov al,[FAT_CLUSTER+bx+si]
		inc si
		cmp al,0x20
		je convertShortNameCont
		mov [fileName83+di],al
		inc di
		cmp si,8
		jb convertShortName

		convertShortNameCont:
		mov si,8
		push di
		inc di

		convertShortNameExt:
		mov al,[FAT_CLUSTER+bx+si]
		inc si
		cmp al,0x20
		je convertShortNameCont2
		mov [fileName83+di],al
		inc di
		cmp si,11
		jb convertShortNameExt

		convertShortNameCont2:
		mov al,0
		mov [fileName83+di],al
		pop ax
		mov si,ax
		mov dl,0
		mov [fileName83+si],dl
		sub di,ax
		cmp di,1
		je parseShortFileName
		mov di,ax
		mov al,'.'
		mov [fileName83+di],al

		parseShortFileName:
		xor di,di

		parseShortFileNameLoop:
		mov al,byte[fileName83+di] ;nazwa przetransformowana
		mov ah,byte[fileNameBuffer+di] ;nazwa zadana
		cmp ah,al ;jesli sie nie zgadzaja
		jne findDirContinue ;to szukamy dalej
		or al,al ;jesli jest nul na koncu (i w sumie sprawdzilismy troche wyzej i w drugim lancuchu tez byl), to jest ten plik
		jz found
		inc di
		jmp parseShortFileNameLoop ;zapetlamy



	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;TODO: chyba nalezy to zrobic ladniej
	;TODO: this should be more elegant and efficient
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	findDirParseLFN: ;przetwarzanie dlugich nazw


	xor dx,dx ;numer wpisu LFN
	mov cx,-31 ;zaczynamy od zerowergo bajtu LFN

		parseLFNloop:
			mov cx,-31 ;pierwsza czesc jest tutaj
			xor bp,bp
			xor si,si ;indeks pomocniczy
			parseLFNloopChar1: ;pierwsza czesc, w ktorej jest 5 znakow
				mov si,dx
				imul si,-32
				add si,cx
				mov al,[FAT_CLUSTER+bx+si]
				mov si,dx
				imul si,13
				add si,bp
				mov [fileNameBufferLFN+si],al
				or al,al ;jesli jest null
				jz compareLFN ;przetwarzamy dalej
				add cx,2 ;zwiekszamy o 2, bo kazdy znak jest kodowany przez 2 bajty
				inc bp
				cmp bp,5 ;w pierwszej czesci mamy 5 znakow
				jne parseLFNloopChar1

			mov cx,-18 ;druga czesc jest tutaj
			parseLFNloopChar2: ;druga czesc: 6 znakow
				mov si,dx
				imul si,-32
				add si,cx
				mov al,[FAT_CLUSTER+bx+si]
				mov si,dx
				imul si,13
				add si,bp
				mov [fileNameBufferLFN+si],al
				or al,al ;jesli jest null
				jz compareLFN ;przetwarzamy dalej
				add cx,2 ;zwiekszamy o 2, bo kazdy znak jest kodowany przez 2 bajty
				inc bp
				cmp bp,11 ;w drugiej czesci 6 znakow, ale wskaznik mamy ciagly, w poprzedniej czesci bylo 5
				jne parseLFNloopChar2

			mov cx,-4 ;trzecia czesc jest tutaj
			parseLFNloopChar3: ;trzecia czesc: 2 znaki
				mov si,dx
				imul si,-32
				add si,cx
				mov al,[FAT_CLUSTER+bx+si]
				mov si,dx
				imul si,13
				add si,bp
				mov [fileNameBufferLFN+si],al
				or al,al ;jesli jest null
				jz compareLFN ;przetwarzamy dalej
				add cx,2 ;zwiekszamy o 2, bo kazdy znak jest kodowany przez 2 bajty
				inc bp
				cmp bp,13 ;o 2 wiecej znowu
				jne parseLFNloopChar3
				;oczywiscie nikt nigdzie nie napisal, ale wcale nazwa LFN NIE musi byc zakonczona NULLem
				;jesli konczy sie rowno z polem, to wystarczy, ze dany wpis ma ustawiony bit 0x40 w numerze wpisu
				mov si,dx ;sprawdzmy jeszcze, czy jest ustawiony bit ostatniego elementu
				imul si,-32
				mov al,[FAT_CLUSTER+bx+si-32]
				test al,0x40
				jnz compareLFN

			inc dx
			jmp parseLFNloop

		compareLFN:
			xor si,si
			compareLFNloop:
				mov al,[fileNameBufferLFN+si]
				mov ah,[fileNameBuffer+si]
				cmp al,ah ;porownajmy obie sciezki
				jne findDirContinue ;jesli rozne, to dalej szukamy
				or ah,ah ;sprawdzmy, czy koniec lancucha
				jnz compareLFNloopCont ;jesli nie, kontynuujemy
				;ale jesli by byl koniec jednego
				or al,al ;to sprawdzamy, czy drugiego tez
				jz found ;jesli drugiego tez, to znaleziono
				jnz findDirContinue ;a jesli drugiego nie, to nie ta nazwa

				compareLFNloopCont:
				inc si
				jmp compareLFNloop

	found:

	mov dh,0
	jmp fatFindDirExit

	findDirErr1:
	mov dh,1
	jmp fatFindDirExit
	findDirErr2:
	mov dh,2
	jmp fatFindDirExit
	findDirErr3:
	mov dh,3
	jmp fatFindDirExit
	findDirErr4:
	mov dh,4
	jmp fatFindDirExit

	fatFindDirExit:
	pop es
	pop di
	pop ecx
	pop eax
	popf
	ret










;zwraca numer nastepnego klastra odczytany z FATu
;przyjmuje w EAX numer aktualnego klastra
;w BL numer FATu
;zwraca w EDX numer kolejnego klastra
fatCheckNextCluster:
	pushf
	push eax
	push ecx
	push ebx
	push edi
	push es
	push esi

	;w FAT32 przy 512B sektorze mamy 128 wpisow FAT na sektor
	;szukamy sektora, w ktorym jest wpis
	xor edx,edx
	mov edi,128
	div edi ;eax/edi, a to da wynik w eax i reszte w edx
	;w eax mamy numer sektora, a w edx pozostala liczbe wpisow
	push edx ;zapiszmy edx na stosie

	cmp bl,0
	jne selectFat2 ;jesli wybrane jest cos innego niz FAT pierwszy (zerowy), to skaczemy dalej

	add eax,[fatFATSector] ;zaczynamy w FAT 1
	jmp fatCCcont

	selectFat2:
	add eax,[fatFAT2Sector] ;zaczynamy w FAT 2

	fatCCcont:
	mov ecx,eax
	mov al,[fatDiskNumber]
	xor bx,bx
	mov es,bx
	mov bx,FAT_TEMP
	mov edx,1
	call _disk_load

	pop ebx ;sciagamy numer klastra
	shl ebx,2 ;mnozenie razy 4, bo jeden wpis zajmuje 4 bajty
	add ebx,FAT_TEMP
	mov edx,dword[ebx] ;w edx juz mamy wyjsciowa wartosc

	pop esi
	pop es
	pop edi
	pop ebx
	pop ecx
	pop eax
	popf
	ret



;czyta wybrany klaster
;dane zachowuje w FAT_CLUSTER
;ECX - numer klastra (>=2)
;zwraca w DH kod bledu: 0 - powodzenie, 1 - niepowodzenie
fatReadCluster:
	pushf
	push esi
	push eax
	push ecx
	push ebx
	push es
	push edi

	cmp ecx,2 ;numer klastra poczatkowego musi byc >=2
	jb fatReadError

	sub ecx,2 ;klaster 0 i 1 fizycznie nie istnieje, a tak naprawde klaster 2 to 0 na dysku

	xor edx,edx
	mov dl,[fatSecPerClus] ;sektory na klaster, razy 1 klaster

	xor eax,eax
	mov al,[fatSecPerClus] ;sektory na klaster
	imul eax,ecx ;razy ilosc klastrow, to ilosc sektorow
	add eax,[fatDataSector] ;+ poczatkowa ilosc sektorow
	mov ecx,eax ;to bezwzgledny adres
	xor bx,bx
	mov es,bx
	mov bx,FAT_CLUSTER
	mov al,[fatDiskNumber]
	call _disk_load ;czytamy klastry

	xor dh,dh
	jmp fatReadOk

	fatReadError:
		mov dh,1
	fatReadOk:
	pop edi
	pop es
	pop ebx
	pop ecx
	pop eax
	pop esi
	popf
	ret


FAT_TEMP equ 0x800
FAT_CLUSTER equ 0x2800

fatDiskNumber db 0 ;numer dysku
fatSecPerClus db 0 ;sektory na klaster
fatBytesPerClus dw 0 ;bajty na klaster
fatFATSector dd 0 ;pierwszy sektor z FATem
fatFAT2Sector dd 0 ;pierwszy sektor z zapasowym FATem
fatDataSector dd 0 ;pierwszy sektor z danymi
fatFirstDataCluster dd 0 ;pierwszy klaster z danymi

fatCurrDirCluster dd 0 ;klaster z folderem, w ktorym aktualnie sie znajdujemy

fileNameBuffer times 256 db 0 ;bufor na nazwe pliku/folderu
fileNameBufferLFN times 256 db 0 ;bufor na nazwe pliku/folderu z LFN
fileName83 times 15 db 0
pathIdx dw 0
requiredType dw 0

msg db 0xd,0xa,0
msg2 db "A",0xd,0xa,0
