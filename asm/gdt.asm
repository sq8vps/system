;This file contains the GDT

gdt_start:

gdt_null: ;null descriptor
dd 0x0
dd 0x0 

gdt_code: ;deskryptor dla segmentu kodu
dw 0xffff ;limit
dw 0x0 ;baza mlodsze slowo
db 0x0 ;baza starszy bajt
db 10011010b ;flagi
			 ;1 - obecny w pamieci, 00 - najwyzsze uprawnienia, 1 - kod lub dane
			 ;1 - kod (0 dane), 0 - segment nizej uprzywilejowany nie moze uzywac tego, 1 - odczytywalny (0 tylko wykonywalny), 0 - nieistotne
db 11001111b ;flagi i starszy bajt limitu
			 ;1 - granurality (mnozenie limitu przez 4k), 1 - 32bit (0 16-bit), 0 - 64bit, 0 - AVL (nieuzywane)
db 0x0 ;baza najstarszy bajt

gdt_data: ;deskryptor dla segmentu danych
dw 0xffff ;limit
dw 0x0 ;baza mlodsze slowo
db 0x0 ;baza starszy bajt
db 10010010b ;flagi
			 ;1 - obecny w pamieci, 00 - najwyzsze uprawnienia, 1 - kod lub dane
			 ;0 - dane (1 kod), 0 - expands down, 1 - wykonywalny, 0 - nieistotne
db 11001111b ;flagi i starszy bajt limitu
db 0x0 ;baza najstarszy bajt


gdt_end:
gdt_descriptor:
dw gdt_end - gdt_start - 1 ;rozmiar GDT
dd gdt_start ;adres GDT 

CODE_SEG equ gdt_code - gdt_start ;stale przesuniecia segmentow
DATA_SEG equ gdt_data - gdt_start
