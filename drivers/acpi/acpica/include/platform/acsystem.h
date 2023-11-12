#ifndef __ACSYSTEM_H__
#define __ACSYSTEM_H__

#include "kernel.h"
// #define memcpy CmMemcpy
// #define memset CmMemset
// #define strlen CmStrlen
// #define strcmp CmStrcmp
// #define strncmp CmStrncmp
// #define memcmp CmMemcmp
// #define strcpy CmStrcpy
// #define strncpy CmStrncpy
// #define strcat CmStrcat
// #define isprint CmIsprint
// #define isxdigit CmIsxdigit
// #define isdigit CmIsdigit
// #define tolower CmTolower
// #define toupper CmToupper
// #define isspace CmIsspace

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
