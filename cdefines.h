#ifndef CDEFINES_H_
#define CDEFINES_H_

/**
 * @file cdefines.h
 * @brief Common defines and data structures for kernel and bootloader
*/

#include <stdint.h>

#define KERNEL_FILE_NAME "kernel32.elf"

#define MM_MEMORY_SIZE (((uint64_t)1 << 32))
#define MM_PAGE_SIZE 4096
#define MM_KERNEL_ADDRESS 0xD0000000
#define MM_DRIVERS_START_ADDRESS 0xE8000000
#define MM_DRIVERS_MAX_SIZE 0x10000000
#define MM_DYNAMIC_START_ADDRESS 0xF8000000
#define MM_DYNAMIC_MAX_SIZE 0x7800000
#define MM_KERNEL_SPACE_START (MM_KERNEL_ADDRESS)
#define MM_KERNEL_SPACE_SIZE (MM_MEMORY_SIZE - MM_KERNEL_SPACE_START)

#define MM_BIOS_MEMORY_MAP_MAX_ENTRIES 256
#define MM_BIOS_MEMORY_MAP_USABLE_REGION_ATTRIBUTES 0x100000001
struct BIOSMemoryMap_s 
{
    uint64_t base;
    uint64_t length;
    uint64_t attributes;
} __attribute__ ((packed));

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
    struct BIOSMemoryMap_s *biosMemoryMap; //pointer to BIOS memory map stored by the 2nd stage bootloader
    uint16_t biosMemoryMapSize; //number of entries in BIOS memory map
    uintptr_t initrdAddress; //initial ramdisk virtual address
    uintptr_t initrdSize; //initial ramdisk size (size of memory allocated for initrd)
};

#if MM_DYNAMIC_START_ADDRESS & (MM_PAGE_SIZE - 1)
    #error Dynamic kernel memory address must be page aligned
#endif
#if MM_DYNAMIC_MAX_SIZE & (MM_PAGE_SIZE - 1)
    #error Dynamic kernel memory size must be page aligned
#endif

#endif