NAME = libdrvcommon.a
CC = i686-elf-gcc
LNK = i686-elf-gcc
ASM = nasm
CC_FLAGS = -I . -c -ffreestanding -masm=intel
ASM_FLAGS = -f elf32
LNK_FLAGS = -nostdlib -r -ffreestanding -z noexecstack
LNK_STATIC_LIBS = -lgcc

CC_SOURCES = $(wildcard *.c)
CC_HEADERS = $(wildcard *.h)
ASM_SOURCES = $(wildcard *.asm)
ASM_INCLUDES = $(wildcard *.inc)
CC_OBJ = ${CC_SOURCES:.c=.o}
ASM_OBJ = ${ASM_SOURCES:.asm=.o}

all: $(NAME)

%.o : %.c $(CC_HEADERS)
	$(CC) $(CC_FLAGS) -o $@ $< 
	
%.o : %.asm $(ASM_INCLUDES)
	$(ASM) $< -o $@ $(ASM_FLAGS)

$(NAME) : $(CC_OBJ) $(ASM_OBJ)
	$(LNK) $(LNK_FLAGS) -o $@ $^ $(LNK_STATIC_LIBS)
	
clean:
	rm -fr  *.o *.gch $(NAME)