#ifndef KERNEL_DEFINES_H_
#define KERNEL_DEFINES_H_

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
#include "export.h"

/**
 * @ingroup defines
 * @{
*/

#define MAX_CPU_COUNT 64
#define MAX_IOAPIC_COUNT 4

#define _STRINGIFY(...) #__VA_ARGS__
#define STRINGIFY(...) _STRINGIFY(__VA_ARGS__)

/**
 * @brief Kernel status codes
*/
typedef enum
{
    //OK response
    OK = 0,

    //common errors
    NULL_POINTER_GIVEN = 1,

    //interrupt module errors
    IT_BAD_VECTOR = 0x00001000, //bad interrupt vector number
    IT_NO_CONTROLLER_CONFIGURED,
    IT_ALREADY_REGISTERED,
    IT_NOT_REGISTERED,
    IT_NO_FREE_VECTORS,

    //memory management module errors
    MM_NO_MEMORY = 0x00002000, //out of physical memory
    MM_PAGE_NOT_PRESENT, //page not present in physical memory
    MM_ALREADY_MAPPED, //page is already mapped to other virtual address
    MM_TOO_MANY_ENTRIES, //too many entries to be inserted into some table
    MM_UNALIGNED_MEMORY, //memory is not aligned correctly to perform the operation
    MM_DYNAMIC_MEMORY_INIT_FAILURE, //dynamically mapped memory module initialization failure
    MM_GDT_ENTRY_LIMIT_EXCEEDED, 
    MM_ALREADY_UNMAPPED, //memory is already unmapped
    MM_HEAP_ALLOCATION_FAILURE,
    MM_DYNAMIC_MEMORY_ALLOCATION_FAILURE,

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
    KE_SCHEDULER_INITIALIZATION_FAILURE,
    KE_TCB_PREPARATION_FAILURE,

    MP_FLOATING_POINTER_TABLE_NOT_FOUND = 0x01000000,
    MP_CONFIGURATION_TABLE_NOT_FOUND,
    MP_CONFIGURATION_TABLE_BROKEN,

    APIC_LAPIC_NOT_AVAILABLE = 0x02000000,
    APIC_IOAPIC_NOT_AVAILABLE,
    APIC_LAPIC_BAD_LINT_NUMBER,
    APIC_IOAPIC_BAD_INPUT_NUMBER,

    ACPI_RSDT_NOT_FOUND = 0x03000000,
    ACPI_CHECKSUM_VALIDATION_FAILED,
    ACPI_MADT_NOT_FOUND,

    BOOTVGA_INIT_FAILURE,

    HAL_INIT_FAILURE,

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
 * @brief Align value up
 * @param val Value to be aligned
 * @param align Alignment value
*/
#define ALIGN_UP(val, align) (val + ((val & (align - 1)) ? (align - (val & (align - 1))) : 0))

/**
 * @brief Align value down
 * @param val Value to be aligned
 * @param align Alignment value
*/
#define ALIGN_DOWN(val, align) ((val - ((val & (align - 1)) ? (val & (align - 1)) : 0)))

/**
 * @brief Macro for inline assembly
*/
#define ASM asm volatile


#ifdef DEBUG
    #include "common.h"
    #define PRINT(...) {CmPrintf(__VA_ARGS__);}
    #define ERROR(...) {CmPrintf(__FILE__ ":" STRINGIFY(__LINE__) ": " __VA_ARGS__);}
    #define BP() {asm volatile("xchg bx,bx");}
    #define RETURN(val) {ERROR("returned 0x%X\n", (unsigned int)val); return val;}
#else
    #define PRINT(...)
    #define ERROR(...)
    #define BP()
    #define RETURN(val) {return val;}
#endif




/**
 * @}
*/

#endif