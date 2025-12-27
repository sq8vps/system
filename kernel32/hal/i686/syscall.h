/**
 * @file syscall.h
 * @brief Low-level syscall handling
 * @ingroup i686
 */

#ifndef I686_SYSCALL_H_
#define I686_SYSCALL_H_

#include "defines.h"

/**
 * @addtogroup i686
 * @{
 */

/**
 * @brief Initialize per-CPU sysenter/syscall mechanism
 */
INTERNAL void I686InitializeSyscall(void);

/**
 * @}
 */

#endif