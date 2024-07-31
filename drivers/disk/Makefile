NAME = disk.drv
C = i686-elf-gcc
CPP = i686-elf-g++
LNK = i686-elf-g++
AR = i686-elf-ar
ASM = nasm

API = ../../api

C_FLAGS = -I. -I.. -I$(API) -c -g -ffreestanding -masm=intel -D DEBUG -Wall -DDRIVER_NAME='"$(NAME)"' -Og
CPP_FLAGS = -I. -I.. -I$(API) -c -g -ffreestanding -masm=intel -D DEBUG -Wall -DDRIVER_NAME='"$(NAME)"' -Og
ASM_FLAGS = -f elf32 -D DEBUG -g -F dwarf -DDRIVER_NAME='"$(NAME)"'
LNK_FLAGS = -r -g -nostdlib -ffreestanding -z noexecstack
LNK_STATIC_LIBS = -lgcc

API_HEADERS = $(wildcard $(API)/**/*.h) $(wildcard $(API)/*.h) $(wildcard $(API)/**/**/*.h) $(wildcard $(API)/**/**/**/*.h) $(wildcard $(API)/**/**/**/**/*.h)

C_SOURCES = $(wildcard **/*.c) $(wildcard *.c) $(wildcard **/**/*.c) $(wildcard **/**/**/*.c) $(wildcard **/**/**/**/*.c)
CPP_SOURCES = $(wildcard **/*.cpp) $(wildcard *.cpp) $(wildcard **/**/*.cpp) $(wildcard **/**/**/*.cpp) $(wildcard **/**/**/**/*.cpp)
C_HEADERS = $(wildcard **/*.h) $(wildcard *.h) $(wildcard **/**/*.h) $(wildcard **/**/**/*.h) $(wildcard **/**/**/**/*.h)
CPP_HEADERS = $(wildcard **/*.hpp) $(wildcard *.hpp) $(wildcard **/**/*.hpp) $(wildcard **/**/**/*.hpp) $(wildcard **/**/**/**/*.hpp)
ASM_SOURCES = $(wildcard **/*.asm) $(wildcard *.asm) $(wildcard **/**/*.asm) $(wildcard **/**/**/*.asm) $(wildcard **/**/**/**/*.asm)
ASM_INCLUDES = $(wildcard **/*.inc) $(wildcard *.inc) $(wildcard **/**/*.inc) $(wildcard **/**/**/*.inc) $(wildcard **/**/**/**/*.inc)
C_OBJ = ${C_SOURCES:.c=.o}
CPP_OBJ = ${CPP_SOURCES:.cpp=.o}
ASM_OBJ = ${ASM_SOURCES:.asm=.o}

all: $(NAME)

%.o : %.c $(C_HEADERS) $(API_HEADERS)
	$(C) $(C_FLAGS) -o $@ $< 

%.o : %.cpp $(CPP_HEADERS)
	$(CPP) $(CPP_FLAGS) -o $@ $< 

%.o : %.asm $(ASM_INCLUDES)
	$(ASM) $< -o $@ $(ASM_FLAGS)

$(NAME) : $(C_OBJ) $(CPP_OBJ) $(ASM_OBJ)
	$(LNK) $(LNK_FLAGS) -o $@ $^ $(LNK_STATIC_LIBS)
	objcopy --only-keep-debug $@ ${@:.drv=.sym}
	objcopy --strip-debug $@
	cp $@ ../../output/
	cp ${@:.drv=.sym} ../../symbols/
	
clean:
	rm -rf $(C_OBJ) $(CPP_OBJ) $(ASM_OBJ) *.gch $(NAME)