NAME = pci.drv
CC = i686-elf-gcc
LNK = i686-elf-gcc
ASM = nasm
CC_FLAGS = -I. -I../../api -c -ffreestanding -masm=intel -Wall -DDRIVER_NAME='"$(NAME)"'
ASM_FLAGS = -f elf32
LNK_FLAGS = -r -nostdlib -ffreestanding -z noexecstack
LNK_STATIC_LIBS = -lgcc

CC_SOURCES = $(wildcard **/*.c) $(wildcard *.c) $(wildcard **/**/*.c)
CC_HEADERS = $(wildcard **/*.h) $(wildcard *.h) $(wildcard **/**/*.h)
ASM_SOURCES = $(wildcard **/*.asm) $(wildcard *.asm) $(wildcard **/**/*.asm)
ASM_INCLUDES = $(wildcard **/*.inc) $(wildcard *.inc) $(wildcard **/**/*.inc)
CC_OBJ = ${CC_SOURCES:.c=.o}
ASM_OBJ = ${ASM_SOURCES:.asm=.o}

all: $(NAME)

%.o : %.c $(CC_HEADERS)
	$(CC) $(CC_FLAGS) -o $@ $< 
	
%.o : %.asm $(ASM_INCLUDES)
	$(ASM) $< -o $@ $(ASM_FLAGS)

$(NAME) : $(CC_OBJ) $(ASM_OBJ)
	$(LNK) $(LNK_FLAGS) -o $@ $^ $(LNK_STATIC_LIBS)
	cp $@ ../../output/
	
clean:
	rm -f $(CC_OBJ) $(ASM_OBJ) *.gch $(NAME)

