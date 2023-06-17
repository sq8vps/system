#ifndef CDEFINES_H_
#define CDEFINES_H_

/**
 * @file cdefines.h
 * @brief Common defines and data structures for kernel and bootloader
*/

#include <stdint.h>

#define KERNEL_FILE_NAME "kernel32.elf"

#define MM_PAGE_SIZE 4096
#define MM_KERNEL_ADDRESS 0xD0000000
#define MM_DRIVERS_START_ADDRESS 0xE8000000
#define MM_DRIVERS_MAX_SIZE 0x10000000
#define MM_OTHER_START_ADDRESS 0xF8000000
#define MM_OTHER_MAX_SIZE 0x7C00000

#define KERNEL_RAW_ELF_ENTRY_NAME_SIZE 20 //max size of file name in raw ELF file entry

/**
 * @brief Entry for raw ELF file which is loaded by the bootloader. Used for bootloader->kernel communication
*/
struct KernelRawELFEntry
{
    char name[KERNEL_RAW_ELF_ENTRY_NAME_SIZE];
    uintptr_t vaddr;
    uintptr_t size;
};

/**
 * @brief Parameters passed to the kernel by the bootloader
*/
struct KernelEntryArgs
{
	uintptr_t kernelPageDir;
	uintptr_t pageUsageTable;
    struct KernelRawELFEntry *rawElfTable;
    uintptr_t rawElfTableSize;
} __attribute__ ((packed));



#endif