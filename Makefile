all: boot.bin loader.bin

boot.bin : asm/boot.asm asm/*.asm
	nasm $< -f bin -o output/$@
	
loader.bin : asm/loader.asm asm/*.asm
	nasm $< -f bin -o output/$@

clean:
	rm -fr *.bin *.dis *.o *.elf