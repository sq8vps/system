C_SOURCES = $(wildcard *.c)
HEADERS = $(wildcard *.h)
# TODO: Make  sources  dep on all  header  files.
# Convert  the *.c filenames  to *.o to give a list of  object  files  to  build
OBJ = ${C_SOURCES:.c=.o}
# Defaul  build  target
all: LDR32 clean
debug: clean LDR32

%.o : %.c %.h
	gcc -ffreestanding -m32 -masm=intel -fno-pie -c $^

LDR32 : ${OBJ}
	ld -o $@ -T loader.ld -Ttext 0xA000 $^
	
clean:
	rm -fr  *.o *.gch