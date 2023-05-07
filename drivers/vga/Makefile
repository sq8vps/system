NAME = libvga.elf
CC = gcc
LNK = ld
ASM = nasm
CC_FLAGS = -I . -mgeneral-regs-only -ffreestanding -m32 -masm=intel
ASM_FLAGS = -f elf32
LNK_FLAGS = -m elf_i386 -r
LNK_STATIC_LIBS = ../hal/libhal.elf ../libcommon.elf

CC_SOURCES = $(wildcard *.c)
CC_HEADERS = $(wildcard *.h)
ASM_SOURCES = $(wildcard *.asm)
ASM_INCLUDES = $(wildcard *.inc)
CC_OBJ = ${CC_SOURCES:.c=.o}
ASM_OBJ = ${ASM_SOURCES:.asm=.o}

all: $(NAME)

%.o : %.c $(CC_HEADERS)
	$(CC) -c $(CC_FLAGS) $< 
	
%.o : %.asm $(ASM_INCLUDES)
	$(ASM) $< -o $@ $(ASM_FLAGS)

$(NAME) : $(CC_OBJ) $(ASM_OBJ)
	$(LNK) $(LNK_FLAGS) -o $@ $^ $(LNK_STATIC_LIBS)
	cp $@ ../../output/drivers
	
clean:
	rm -fr  *.o *.gch $(NAME)