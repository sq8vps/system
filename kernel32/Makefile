NAME = kernel32.elf
CC = i686-elf-gcc
LNK = i686-elf-gcc
AR = i686-elf-ar
ASM = nasm
CC_FLAGS = -I. -I.. -c -g -ffreestanding -masm=intel -D DEBUG -Wall
ASM_FLAGS = -f elf32 -D DEBUG
LNK_FLAGS = -g -nostdlib -T kernel.ld -ffreestanding -z noexecstack
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
	objcopy --only-keep-debug $@ ${@:.elf=.sym}
	objcopy --strip-debug $@
	cp $@ ../output/
	cp ${@:.elf=.sym} ../symbols/
	
clean:
	rm -rf $(CC_OBJ) $(ASM_OBJ) *.gch $(NAME)

