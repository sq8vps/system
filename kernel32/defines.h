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

#include "../cdefines.h"
#include <stddef.h>

/**
 * @ingroup defines
 * @{
*/

#define MAX_CPU_COUNT 64

#define STRINGIFY(s) #s
#define EXPAND_AND_STRINGIFY(s) STRINGIFY(s)

/**
 * @brief Kernel status codes
*/
typedef enum _STATUS
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
    MM_ALREADY_UNMAPPED, //memory is already unmapped
    MM_HEAP_ALLOCATION_FAILURE,

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
    EXEC_PROCESS_PAGE_DIRECTORY_CREATION_FAILURE,

    KE_TSS_ENTRY_LIMIT_EXCEEDED = 0x00004000,
    KE_NO_EXECUTABLE_TASK, //no task to execute - should never happen, there always should be the idle task
    KE_UNACQUIRED_MUTEX_RELEASED,
    KE_SCHEDULER_INITIALIZATION_FAILURE,
    KE_TCB_PREPARATION_FAILURE,

} STATUS;

/**
 * @brief General privilege level enum
*/
typedef enum PrivilegeLevel_t
{
    PL_KERNEL,
    PL_USER
} PrivilegeLevel_t;

/**
 * @brief Macro for exporting the symbol to be used by kernel modules
*/
#define EXPORT __attribute__((visibility("default")))

/**
 * @brief Attribute for never-returning functions
*/
#define NORETURN __attribute__((noreturn))

/**
 * @brief Attribute for packed structures
*/
#define PACKED __attribute__ ((packed))

/**
 * @brief Variable alignment macro
 * @param n Alignment value in bytes
*/
#define ALIGN(n) __attribute__ ((aligned(n)))

/**
 * @brief Align value
 * @param val Value to be aligned
 * @param align Alignment value
*/
#define ALIGN_VAL(val, align) (val + ((val & (align - 1)) ? (align - (val & (align - 1))) : 0))


#ifdef DEBUG
    #include "../drivers/vga/vga.h"
    #define PRINT(...) {printf(__VA_ARGS__);}
    #define ERROR(...) {printf(__FILE__ ":" EXPAND_AND_STRINGIFY(__LINE__) ": " __VA_ARGS__);}
    #define BP() {asm volatile("xchg bx,bx");};
#else
    #define PRINT(...)
    #define ERROR(...)
    #define BP()
#endif




/**
 * @}
*/

#endif