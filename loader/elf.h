#ifndef LOADER_ELF_H
#define LOADER_ELF_H

#include <stdint.h>
#include "defines.h"

error_t Elf_load(uint8_t *name, uint32_t *entryPoint);

#endif
