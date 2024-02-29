//This header file is generated automatically
#ifndef EXPORTED___API__DEFINES_H_
#define EXPORTED___API__DEFINES_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "../cdefines.h"
#include <stddef.h>
/**
 * @brief Mark symbol (function/variable) as internal/hidden
*/
#define INTERNAL __attribute__ ((visibility("hidden")))

/**
 * @brief Stringify without expanding
 * @param ... Arguments to stringify
 * @return Stringified arguments
*/
#define _STRINGIFY(...) #__VA_ARGS__

/**
 * @brief Expand and stringify
 * @param ... Arguments to expand and stringify
 * @return Expanded and stringified arguments
*/
#define STRINGIFY(...) _STRINGIFY(__VA_ARGS__)

/**
 * @brief Kernel status codes
*/
typedef enum
{
    //OK response
    OK = 0,

    //general non-error responses
    INTERRUPT_NOT_HANDLED,
    INTERRUPT_FINISHED,
    INTERRUPT_DEFERRED,

    //common errors
    NULL_POINTER_GIVEN = 0x100,
    NOT_IMPLEMENTED,
    OUT_OF_RESOURCES,
    DEVICE_NOT_AVAILABLE,
    SYSTEM_INCOMPATIBLE,
    BAD_PARAMETER,
    OPERATION_NOT_ALLOWED,
    BAD_ALIGNMENT,
    NOT_COMPATIBLE,

    //interrupt module errors
    IT_BAD_VECTOR = 0x00001000, //bad interrupt vector number
    IT_VECTOR_NOT_FREE,
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
    EXEC_ELF_UNDEFINED_externAL_SYMBOL,
    EXEC_UNSUPPORTED_KERNEL_MODE_DRIVER_TYPE,
    EXEC_BAD_DRIVER_INDEX,
    EXEC_BAD_DRIVER_CLASS,
    EXEC_PROCESS_PAGE_DIRECTORY_CREATION_FAILURE,
    EXEC_DRIVER_INIT_FAILED,

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
    IO_VFS_INITIALIZATION_FAILED,
    IO_RP_NOT_CANCELLABLE,
    IO_RP_PROCESSING_FAILED,
    IO_RP_CODE_UNKNOWN,
    IO_VOLUME_ALREADY_EXISTS,
    IO_VOLUME_ALREADY_MOUNTED,
    IO_DEVICE_INCOMPATIBLE,

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

    UNKNOWN_ERROR = 0xFFFFFFFF,

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
 * @brief A common timestamp type
*/
typedef uint64_t Time_t;

/**
 * @brief A common character type (UTF-8)
*/
typedef char Utf8_t;

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

/**
 * @brief Check if character \a x is alphanumeric
 * @param x Character to check
 * @return 1 if is alphanumeric, 0 otherwise
*/
#define IS_ALPHANUMERIC(x) ((((x) >= '0') && ((x) <= '9')) || (((x) >= 'A') && ((x) <= 'Z')) || (((x) >= 'a') && ((x) <= 'z')))

/**
 * @brief Convert microseconds to nanoseconds (standard kernel time resolution)
 * @param us Value in microseconds
 * @return Value in nanoseconds
*/
#define US_TO_NS(us) (((uint64_t)1000) * (us))

/**
 * @brief Convert milliseconds to nanoseconds (standard kernel time resolution)
 * @param us Value in milliseconds
 * @return Value in nanoseconds
*/
#define MS_TO_NS(ms) (((uint64_t)1000000) * (ms))


#ifdef __cplusplus
}
#endif

#endif