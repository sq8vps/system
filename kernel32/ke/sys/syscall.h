#ifndef KERNEL_SYSCALL_H_
#define KERNEL_SYSCALL_H_

#include "defines.h"
#include <stdint.h>

/**
 * @brief System call handler/wrapper type
 */
typedef reg_t (*KeSyscallHandler)(reg_t, reg_t, reg_t, reg_t, reg_t);

/**
 * @brief Perform system call
 * @param code System call code
 * @param arg1 System call argument 1
 * @param arg2 System call argument 2
 * @param arg3 System call argument 3
 * @param arg4 System call argument 4
 * @param arg5 System call argument 5
 * @return System call result
 */
INTERNAL reg_t KePerformSyscall(reg_t code, reg_t arg1, reg_t arg2, reg_t arg3, reg_t arg4, reg_t arg5);

#endif