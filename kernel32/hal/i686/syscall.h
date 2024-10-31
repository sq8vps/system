#ifndef I686_SYSCALL_H_
#define I686_SYSCALL_H_

#include "defines.h"

/**
 * @brief Initialize per-CPU sysenter/syscall mechanism
 */
INTERNAL void I686InitializeSyscall(void);

#endif