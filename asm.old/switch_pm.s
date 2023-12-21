[bits 16] ;uzywamy 16 bitow

switch_pm:

cli ;wylaczamy przerwania

lgdt [gdt_descriptor] ;ladujemy tablice deskryptorow GDT
	
mov eax,cr0 ;wlaczamy tryb chroniony
or eax,0x01 ;ustawiamy tryb chroniony nie zmieniajac reszty
mov cr0,eax
	
jmp CODE_SEG:init_pm ;skok do inijalizacji trybu chronionego


[bits 32]

init_pm:

mov ax,DATA_SEG ;w trybie chronionym wszystkie rejestry segmentowe musza wskazywac na tabele GDT
mov ds,ax
mov ss,ax
mov es,ax
mov fs,ax
mov gs,ax

mov eax,0x5000 ;adres stosu na gorze wolnej pamieci
mov esp,eax

call BEGIN_PM ;dalsza czesc uruchamiania
