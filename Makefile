all: boot.bin loader.bin

boot.bin : asm/boot.asm asm/*.asm
	nasm $< -f bin -o $@
	
loader.bin : asm/loader.asm asm/*.asm
	nasm $< -f bin -o $@

clean:
	rm -fr *.bin *.dis *.o *.elf