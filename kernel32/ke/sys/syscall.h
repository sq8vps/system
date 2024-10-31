#ifndef KERNEL_SYSCALL_H_
#define KERNEL_SYSCALL_H_

#include "defines.h"
#include <stdint.h>

/**
 * @brief Perform system call
 * @param code System call code
 * @param arg1 System call argument 1
 * @param arg2 System call argument 2
 * @param arg3 System call argument 3
 * @param arg4 System call argument 4
 * @param arg5 System call argument 5
 * @return Status code
 * @attention This function might terminate task when illegal parameters are detected
 */
INTERNAL STATUS KePerformSyscall(uintptr_t code, uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5);

#endif