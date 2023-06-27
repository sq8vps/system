#ifndef KERNEL32_DEFINES_H_
#define KERNEL32_DEFINES_H_

/**
 * @file defines.h
 * @brief Common kernel definitions and macros
 * 
 * Provides a set of common kernel definitions, typedefs and macros.
 * 
 * @defgroup defines Common kernel definitions
*/

/**
 * @ingroup defines
 * @{
*/

#define MAX_CPU_COUNT 64
#define KERNEL_STACK_SIZE_PER_CPU 4096

#define CPU0_KERNEL_STACK_TOP 0xD8000000

#ifndef NULL
#define NULL ((void*)0)
#endif

#define STRINGIFY(s) #s
#define EXPAND_AND_STRINGIFY(s) STRINGIFY(s)

typedef enum kError_t
{
    //OK response
    OK = 0,

    //common errors
    NULL_POINTER_GIVEN = 1,

    //interrupt module errors
    IT_BAD_VECTOR = 0x00001000, //bad interrupt vector number

    //memory management module errors
    MM_NO_MEMORY = 0x00002000, //out of physical memory
    MM_PAGE_NOT_PRESENT, //page not present in physical memory
    MM_ALREADY_MAPPED, //page is already mapped to other virtual address
    MM_TOO_MANY_ENTRIES, //too many entries to be inserted into some table
    MM_KERNEL_PAGE_FAULT, //page fault in kernel mode - panic
    MM_UNALIGNED_MEMORY, //memory is not aligned correctly to perform the operation
    MM_DYNAMIC_MEMORY_INIT_FAILURE, //dynamically mapped memory module initialization failure - panic
    MM_GDT_ENTRY_LIMIT_EXCEEDED, 

    EXEC_ELF_BAD_ARCHITECTURE = 0x00003000,
    EXEC_ELF_BAD_INSTRUCTION_SET,
    EXEC_ELF_BAD_FORMAT,
    EXEC_ELF_BAD_ENDIANESS,
    EXEC_ELF_BROKEN,
    EXEC_ELF_UNSUPPORTED_TYPE,
    EXEC_ELF_UNDEFINED_SYMBOL,
    EXEC_ELF_UNDEFINED_EXTERNAL_SYMBOL,
    EXEC_MALLOC_FAILED,
    EXEC_UNSUPPORTED_KERNEL_MODE_DRIVER_TYPE,
    EXEC_BAD_DRIVER_INDEX,
    EXEC_BAD_DRIVER_CLASS,

    KE_TSS_ENTRY_LIMIT_EXCEEDED = 0x00004000,


} kError_t;

/**
 * @brief Macro for exporting the symbol to be used by kernel modules
*/
#define EXPORT __attribute__((visibility("default")))

/**
 * @brief Macro for never-returning functions
*/
#define NORETURN __attribute__((noreturn))

#define PACKED __attribute__ ((packed))

/**
 * @}
*/

#endif