#ifndef LOADER_ELF_H
#define LOADER_ELF_H

#include <stdint.h>
#include "defines.h"

error_t Elf_loadExec(const uint8_t *name, uint32_t *entryPoint);

error_t Elf_loadRaw(const char *name, uint32_t *vaddr);
error_t Elf_loadDriver(const char *name);
error_t Elf_loadOther(const char *name);
struct KernelRawELFEntry* Elf_getRawELFTable(uintptr_t *tableSize);

#endif
