all: clean boot others

boot: boot.bin loader.bin

boot.bin : asm/boot.asm
	nasm $^ -f bin -o $@
	
loader.bin : asm/loader.asm
	nasm $^ -f bin -o $@

clean:
	rm -fr *.bin *.dis *.o *.elf
	
others:
	cd loader && make