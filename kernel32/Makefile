NAME = kernel32.elf
C = i686-elf-gcc
CPP = i686-elf-g++
LNK = i686-elf-g++
AR = i686-elf-ar
ASM = nasm
C_FLAGS = -I. -I.. -c -g -ffreestanding -masm=intel -D DEBUG -D SMP -Wall -Og
CPP_FLAGS = -I. -I.. -c -g -ffreestanding -masm=intel -D DEBUG -D SMP -Wall -Og -std=c++2b
ASM_FLAGS = -f elf32 -D DEBUG -D SMP -g -F dwarf
LNK_FLAGS = -g -nostdlib -T kernel.ld -ffreestanding -z noexecstack
LNK_STATIC_LIBS = -lgcc

C_SOURCES := $(wildcard **/*.c) $(wildcard *.c) $(wildcard **/**/*.c) $(wildcard **/**/**/*.c) $(wildcard **/**/**/**/*.c)
C_SOURCES := $(filter-out $(C_BOOT_SOURCES), $(C_SOURCES)) 
CPP_SOURCES := $(wildcard **/*.cpp) $(wildcard *.cpp) $(wildcard **/**/*.cpp) $(wildcard **/**/**/*.cpp) $(wildcard **/**/**/**/*.cpp)
C_HEADERS := $(wildcard **/*.h) $(wildcard *.h) $(wildcard **/**/*.h) $(wildcard **/**/**/*.h) $(wildcard **/**/**/**/*.h)
CPP_HEADERS := $(wildcard **/*.hpp) $(wildcard *.hpp) $(wildcard **/**/*.hpp) $(wildcard **/**/**/*.hpp) $(wildcard **/**/**/**/*.hpp)
ASM_SOURCES := $(wildcard **/*.asm) $(wildcard *.asm) $(wildcard **/**/*.asm) $(wildcard **/**/**/*.asm) $(wildcard **/**/**/**/*.asm)
ASM_INCLUDES := $(wildcard **/*.inc) $(wildcard *.inc) $(wildcard **/**/*.inc) $(wildcard **/**/**/*.inc) $(wildcard **/**/**/**/*.inc)
C_OBJ := ${C_SOURCES:.c=.o}
CPP_OBJ := ${CPP_SOURCES:.cpp=.o}
ASM_OBJ := ${ASM_SOURCES:.asm=.o}

all: $(NAME)

%.o : %.c $(C_HEADERS)
	$(C) $(C_FLAGS) -o $@ $< 

%.o : %.cpp $(CPP_HEADERS)
	$(CPP) $(CPP_FLAGS) -o $@ $< 

%.o : %.asm $(ASM_INCLUDES)
	$(ASM) $< -o $@ $(ASM_FLAGS)

$(NAME) : $(C_OBJ) $(CPP_OBJ) $(ASM_OBJ) $(C_BOOT_OBJ)
	$(LNK) $(LNK_FLAGS) -o $@ $^ $(LNK_STATIC_LIBS)
	objcopy --only-keep-debug $@ ${@:.elf=.sym}
	objcopy --strip-debug $@
	cp $@ ../output/
	cp ${@:.elf=.sym} ../symbols/
	
clean:
	rm -rf $(C_OBJ) $(CPP_OBJ) $(ASM_OBJ) *.gch $(NAME)

