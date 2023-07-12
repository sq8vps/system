NAME = tmvga.drv
KERNEL_HEADER_DIR = ../../kernel32
CC = i686-elf-gcc
LNK = i686-elf-gcc
ASM = nasm
CC_FLAGS = -I. -c -ffreestanding -masm=intel -I$(KERNEL_HEADER_DIR)
ASM_FLAGS = -f elf32
LNK_FLAGS = -nostdlib -r -ffreestanding -z noexecstack
LNK_STATIC_LIBS = -lgcc

CC_SOURCES = $(wildcard **/*.c) $(wildcard *.c)
CC_HEADERS = $(wildcard **/*.h) $(wildcard *.h)
ASM_SOURCES = $(wildcard **/*.asm) $(wildcard *.asm)
ASM_INCLUDES = $(wildcard **/*.inc) $(wildcard *.inc)
CC_OBJ = ${CC_SOURCES:.c=.o}
ASM_OBJ = ${ASM_SOURCES:.asm=.o}

all: $(NAME)

%.o : %.c $(CC_HEADERS)
	$(CC) $(CC_FLAGS) -o $@ $< 
	
%.o : %.asm $(ASM_INCLUDES)
	$(ASM) $< -o $@ $(ASM_FLAGS)

$(NAME) : $(CC_OBJ) $(ASM_OBJ)
	$(LNK) $(LNK_FLAGS) -o $@ $^ $(LNK_STATIC_LIBS)
	cp $@ ../../output/drivers
	
clean:
	rm -fr  *.o *.gch $(NAME)