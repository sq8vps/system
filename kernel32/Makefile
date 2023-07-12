NAME = kernel32.elf
CC = i686-elf-gcc
LNK = i686-elf-gcc
ASM = nasm
INCLUDE_SEARCH = ./mm:./ke:./it:./hal:./exec
CC_FLAGS = -I. -I.. -I${INCLUDE_SEARCH} -c -ffreestanding -masm=intel -fvisibility=hidden -D DEBUG
ASM_FLAGS = -f elf32 -D DEBUG
LNK_FLAGS = -nostdlib -T kernel.ld -ffreestanding -z noexecstack -L../output/drivers
LNK_STATIC_LIBS = -lgcc -l:vga.drv

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
	cp $@ ../output/
	
clean:
	rm -f $(CC_OBJ) $(ASM_OBJ) *.gch $(NAME)