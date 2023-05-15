
#include "elf.h"

int main(int argc, char** argv) {
    uint32_t d;
    Elf_getKernelSymbolTable("c:\\programy\\system\\output\\kernel32.elf");
    Elf_load("c:\\programy\\system\\elf\\vga.drv", &d);
}
