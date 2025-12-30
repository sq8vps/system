/**
 * @file exec.h
 * @brief General executable file-related routines
 * @ingroup exec
 */


#ifndef KERNEL_EXEC_H_
#define KERNEL_EXEC_H_

/**
 * @addtogroup exec Executable files- and kernel mode drivers-related definitions and routines
 */

/**
 * @addtogroup exec_exec General executable file-related routines
 * @ingroup exec
 * @kinternal
 * @{
*/

#include <stdint.h>
#include "defines.h"

/**
 * @brief Get executable's required BSS/no-bits section memory size
 * @param *name Executable path
 * @param *size Output size
 * @return Status code
*/
INTERNAL STATUS ExGetExecutableRequiredBssSize(const char *name, size_t *size);

/**
 * @brief Prepare BSS/no-bits sections and update executable header
 * @param *fileStart File start pointer
 * @param *bss Pointer to the space allocated for BSS
 * @return Status code
*/
INTERNAL STATUS ExPrepareExecutableBss(void *fileStart, void *bss);

/**
 * @}
 */

#endif