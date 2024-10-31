#ifndef __ACSYSTEM_H__
#define __ACSYSTEM_H__

#define DISABLE_KERNEL_STDLIB

#include "kernel.h"
// #define memcpy RtlMemcpy
// #define memset RtlMemset
// #define strlen RtlStrlen
// #define strcmp RtlStrcmp
// #define strncmp RtlStrncmp
// #define memcmp RtlMemcmp
// #define strcpy RtlStrcpy
// #define strncpy RtlStrncpy
// #define strcat RtlStrcat
// #define isprint RtlIsprint
// #define isxdigit RtlIsxdigit
// #define isdigit RtlIsdigit
// #define tolower RtlTolower
// #define toupper RtlToupper
// #define isspace RtlIsspace

#define ACPI_SPINLOCK KeSpinlock*
#define ACPI_SEMAPHORE KeSemaphore*
#define ACPI_MUTEX KeMutex*
#define ACPI_MUTEX_TYPE ACPI_OSL_MUTEX

#define ACPI_CACHE_T ACPI_MEMORY_LIST
#define ACPI_USE_LOCAL_CACHE 1

#define ACPI_USE_NATIVE_DIVIDE
//#define ACPI_DEBUG_OUTPUT

#include <stdint.h>

#if defined(__x86_64__)
#define ACPI_MACHINE_WIDTH          64
#define COMPILER_DEPENDENT_INT64    int64_t
#define COMPILER_DEPENDENT_UINT64   uint64_t
#elif defined(__i386__)
#define ACPI_MACHINE_WIDTH          32
#define COMPILER_DEPENDENT_INT64    int64_t
#define COMPILER_DEPENDENT_UINT64   uint64_t
#else
#error Unsupported machine type
#endif

#endif
