#ifndef CDEFINES_H_
#define CDEFINES_H_

/**
 * @file cdefines.h
 * @brief Common defines and data structures for kernel and bootloader
*/

#include <stdint.h>

#define MM_PAGE_SIZE 4096

#define KERNEL_RAW_ELF_ENTRY_NAME_SIZE 20
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