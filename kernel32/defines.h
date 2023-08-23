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

/**
 * @ingroup defines
 * @{
*/

#define MAX_CPU_COUNT 64
#define MAX_IOAPIC_COUNT 4

/**
 * @brief Mark all following lines up to the next blank line as "to be exported"
*/
#define EXPORT

/**
 * @brief Mark all following lines up to the next blank line as "typedef to be exported"
*/
#define EXPORT

/**
 * @brief Mark function/variable to be externed on exporting
*/
#define EXTERN

EXPORT
/**
 * @brief Mark symbol (function/variable) as internal/hidden
*/
#define INTERNAL __attribute__ ((visibility("hidden")))

EXPORT
/**
 * @brief Stringify without expanding
 * @param ... Arguments to stringify
 * @return Stringified arguments
*/
#define _STRINGIFY(...) #__VA_ARGS__

EXPORT
/**
 * @brief Expand and stringify
 * @param ... Arguments to expand and stringify
 * @return Expanded and stringified arguments
*/
#define STRINGIFY(...) _STRINGIFY(__VA_ARGS__)

EXPORT
/**
 * @brief Kernel status codes
*/
typedef enum
{
    //OK response
    OK = 0,

    //common errors
    NULL_POINTER_GIVEN = 1,
    NOT_IMPLEMENTED,
    OUT_OF_RESOURCES,
    DEVICE_NOT_AVAILABLE,

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

    EXEC_ELF_BAD_ARCHITECTURE = 0x00003000,
    EXEC_ELF_BAD_INSTRUCTION_SET,
    EXEC_ELF_BAD_FORMAT,
    EXEC_ELF_BAD_ENDIANESS,
    EXEC_ELF_BROKEN,
    EXEC_ELF_UNSUPPORTED_TYPE,
    EXEC_ELF_UNDEFINED_SYMBOL,
    EXEC_ELF_UNDEFINED_EXTERNAL_SYMBOL,
    EXEC_UNSUPPORTED_KERNEL_MODE_DRIVER_TYPE,
    EXEC_BAD_DRIVER_INDEX,
    EXEC_BAD_DRIVER_CLASS,
    EXEC_PROCESS_PAGE_DIRECTORY_CREATION_FAILURE,

    KE_TSS_ENTRY_LIMIT_EXCEEDED = 0x00004000,
    KE_SCHEDULER_INITIALIZATION_FAILURE,
    KE_TCB_PREPARATION_FAILURE,

    IO_ROOT_DEVICE_INIT_FAILURE = 0x00005000,
    IO_ILLEGAL_NAME,
    IO_FILE_NOT_FOUND,
    IO_FILE_ALREADY_EXISTS,
    IO_NOT_DISK_DEVICE_FILE,
    IO_NOT_A_DIRECTORY,
    IO_FILE_COUNT_LIMIT_REACHED,
    IO_BAD_FILE_TYPE,
    IO_FILE_LOCKED,
    IO_FILE_IRREMOVABLE,
    IO_DIRECTORY_NOT_EMPTY,
    IO_FILE_IN_USE,
    IO_FILE_READ_ONLY,
    IO_READ_INCOMPLETE,

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
    INITRD_INIT_FAILURE,

    HAL_INIT_FAILURE,

} STATUS;

EXPORT
/**
 * @brief General privilege level enum
*/
typedef enum PrivilegeLevel_t
{
    PL_KERNEL,
    PL_USER
} PrivilegeLevel_t;

EXPORT
/**
 * @brief A common timestamp type
*/
typedef uint64_t Time_t;

EXPORT
/**
 * @brief A common character type (UTF-8)
*/
typedef char Utf8_t;

EXPORT
/**
 * @brief Attribute for never-returning functions
*/
#define NORETURN __attribute__((noreturn))

EXPORT
/**
 * @brief Attribute for packed structures
*/
#define PACKED __attribute__ ((packed))

EXPORT
/**
 * @brief Variable alignment macro
 * @param n Alignment value in bytes
*/
#define ALIGN(n) __attribute__ ((aligned(n)))

EXPORT
/**
 * @brief Align value up
 * @param val Value to be aligned
 * @param align Alignment value
*/
#define ALIGN_UP(val, align) (val + ((val & (align - 1)) ? (align - (val & (align - 1))) : 0))

EXPORT
/**
 * @brief Align value down
 * @param val Value to be aligned
 * @param align Alignment value
*/
#define ALIGN_DOWN(val, align) ((val - ((val & (align - 1)) ? (val & (align - 1)) : 0)))

EXPORT
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

EXPORT
/**
 * @brief Check if character \a x is alphanumeric
 * @param x Character to check
 * @return 1 if is alphanumeric, 0 otherwise
*/
#define IS_ALPHANUMERIC(x) ((((x) >= '0') && ((x) <= '9')) || (((x) >= 'A') && ((x) <= 'Z')) || (((x) >= 'a') && ((x) <= 'z')))

EXPORT
/**
 * @brief Convert microseconds to nanoseconds (standard kernel time resolution)
 * @param us Value in microseconds
 * @return Value in nanoseconds
*/
#define US_TO_NS(us) (((uint64_t)1000) * (us))

EXPORT
/**
 * @brief Convert milliseconds to nanoseconds (standard kernel time resolution)
 * @param us Value in milliseconds
 * @return Value in nanoseconds
*/
#define MS_TO_NS(ms) (((uint64_t)1000000) * (ms))

/**
 * @}
*/

#endif