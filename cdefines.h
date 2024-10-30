#ifndef CDEFINES_H_
#define CDEFINES_H_

/**
 * @file cdefines.h
 * @brief Common defines and data structures for kernel and bootloader
*/

#include <stdint.h>
//TODO: move to HAL and remove this file
#define MM_MEMORY_SIZE (((uint64_t)1 << 32))
#define MM_PAGE_SIZE 4096
#define MM_KERNEL_SPACE_START_ADDRESS 0xD0000000
#define MM_KERNEL_ADDRESS 0xD6000000
#define MM_DRIVERS_START_ADDRESS 0xE8000000
#define MM_DRIVERS_MAX_SIZE 0x10000000
#define MM_DYNAMIC_START_ADDRESS 0xF8000000
#define MM_DYNAMIC_MAX_SIZE 0x7800000
#define MM_KERNEL_SPACE_START (MM_KERNEL_ADDRESS)
#define MM_KERNEL_SPACE_SIZE (MM_MEMORY_SIZE - MM_KERNEL_SPACE_START)
#define MM_LOWER_MEMORY_SIZE 0x100000


#if MM_DYNAMIC_START_ADDRESS & (MM_PAGE_SIZE - 1)
    #error Dynamic kernel memory address must be page aligned
#endif
#if MM_DYNAMIC_MAX_SIZE & (MM_PAGE_SIZE - 1)
    #error Dynamic kernel memory size must be page aligned
#endif

#endif