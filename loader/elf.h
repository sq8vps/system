#ifndef LOADER_ELF_H
#define LOADER_ELF_H

#include <stdint.h>
#include "defines.h"

error_t Elf_loadExec(const uint8_t *name, uintptr_t *entryPoint);

#endif
