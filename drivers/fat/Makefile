NAME = fat.drv
CC = i686-elf-gcc
LNK = i686-elf-gcc
ASM = nasm
CC_FLAGS = -I. -I../../api -c -g -ffreestanding -masm=intel -Wall -DDRIVER_NAME='"$(NAME)"'
ASM_FLAGS = -f elf32
LNK_FLAGS = -r -g -nostdlib -ffreestanding -z noexecstack
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
	objcopy --only-keep-debug $@ ${@:.drv=.sym}
	objcopy --strip-debug $@
	cp $@ ../../output/
	cp ${@:.drv=.sym} ../../symbols/
	
clean:
	rm -f $(CC_OBJ) $(ASM_OBJ) *.gch $(NAME)

