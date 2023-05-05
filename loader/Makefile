CC = gcc
LNK = ld
CFLAGS = -I .

C_SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)
OBJ = ${C_SOURCES:.c=.o}


all: LDR32

%.o : %.c $(HEADERS)
	$(CC) -ffreestanding -m32 -masm=intel -fno-pie -c $< $(CFLAGS)

LDR32 : ${OBJ}
	$(LNK) -o $@ -T loader.ld -Ttext 0xA000 $^
	cp $@ ../output
	
clean:
	rm -fr  *.o *.gch LDR32