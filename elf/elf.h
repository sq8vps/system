#ifndef LOADER_ELF_H
#define LOADER_ELF_H

#include <stdint.h>

typedef enum STATUS
{
    OK,


    EXEC_ELF_BAD_ARCHITECTURE,
    EXEC_ELF_BAD_INSTRUCTION_SET,
    EXEC_ELF_BAD_FORMAT,
    EXEC_ELF_BAD_ENDIANESS,
    EXEC_ELF_BROKEN,
    EXEC_ELF_UNSUPPORTED_TYPE,
    EXEC_ELF_UNDEFINED_EXTERNAL_SYMBOL,

} STATUS;

STATUS Elf_load(char *name, uint32_t *entryPoint);
STATUS Elf_getKernelSymbolTable(const char *path);

#endif
