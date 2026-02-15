#ifndef KERNEL_SYS_SYSCALL_H_
#define KERNEL_SYS_SYSCALL_H_

#include "defines.h"

/*
 * Do not include this file. This file is used only for internal syscall handling and for Nabla API export.
 */

#ifndef __KERNEL_INTERNAL
NABLA_API
/**
 * @brief Define system call enumeration constant
 * @param name System call name
 * @param index System call number
 * @attention This is for internal use.
 * @note To get system call number, look in \ref ApiSyscallCode or use \ref GET_SYSCALL_NUMBER()
 */
#define SYSCALL(name, number) Sys##name = number,
END_NABLA_API
#endif

#ifdef __KERNEL_INTERNAL
#include "llsyscall.h"

#define SYSCALL(name, index) extern const struct KeSyscallDescriptor SYSCALL_DESC_NAME(name);
#include "syscall.def"
#undef SYSCALL

#define SYSCALL(name, index) [index] = &SYSCALL_DESC_NAME(name),

const struct KeSyscallDescriptor *const KeSyscallDescriptorTable[] =
#endif

NABLA_API
#ifndef __KERNEL_INTERNAL
enum ApiSyscallCode
#endif
{
    #include "syscall.def"
};


#ifndef __KERNEL_INTERNAL


/**
 * @brief Get system call number based on it's name
 * @param name System call name
 * @return Associated system call number
 * @note This is a macro which should return values from \ref ApiSyscallCode. Note there is no error checking.
 */
#define GET_SYSCALL_NUMBER(name) (Sys##name)

#endif

/**
 * @brief No operation syscall
 * @param unused1 Unused
 * @param unused2 Unused
 * @param unused3 Unused
 * @param unused4 Unused
 * @param unused5 Unused
 * @return Always OK
 */
STATUS ApiNoOperation(uint32_t unused1, uint16_t unused2, uint8_t unused3, void *unused4, uint64_t unused5);

/**
 * @brief Option definitions for \ref ApiGetSystemConfig()
 */
enum ApiSystemConfigParam
{
    API_SYSTEM_CONFIG_PAGE_SIZE = 0, /**< Page size */
};

/**
 * @brief Get system configuration parameter
 * @param param Parameter to obtain the value for (look \ref ApiSystemConfigParam)
 * @param *output Pointer to store the parameter to
 * @return \a OK on success
 * @return \a BAD_PARAMETER \a output points to an illegal memory
 * @return \a NOT_IMPLEMENTED when \a param is unknown
 */
STATUS ApiGetSystemConfig(enum ApiSystemConfigParam param, uint64_t *output);

END_NABLA_API

#endif