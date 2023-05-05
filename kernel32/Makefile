CC = gcc
LNK = ld
ASM = nasm
CFLAGS = -I . -mgeneral-regs-only
ASMFLAGS = -f elf32
LDSCRIPT = kernel.ld

C_SOURCES = $(wildcard *.c)
ASM_SOURCES = $(wildcard *.asm)
ASM_INCLUDES = $(wildcard *.inc)
HEADERS = $(wildcard *.h)
COBJ = ${C_SOURCES:.c=.o}
ASMOBJ = ${ASM_SOURCES:.asm=.o}

all: kernel32.elf

%.o : %.c $(HEADERS)
	$(CC) -ffreestanding -m32 -masm=intel -fno-pie -c $< $(CFLAGS)
	
%.o : %.asm $(ASM_INCLUDES)
	$(ASM) $< -o $@ $(ASMFLAGS)

kernel32.elf : ${COBJ} ${ASMOBJ}
	$(LNK) -o $@ -T $(LDSCRIPT) $^
	cp $@ ../output
	
clean:
	rm -fr  *.o *.gch kernel32.elf