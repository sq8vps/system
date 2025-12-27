/**
 * @file load.h
 * @brief Executable file loading routines
 * @ingroup exec
 */

#ifndef KERNEL_LOAD_H_
#define KERNEL_LOAD_H_

#include <stdint.h>
#include "defines.h"

/**
 * @addtogroup ex_load Executable loading routines
 * @ingroup exec
 * @{
*/

/**
 * @brief Load process image
 * @param *path Image path
 * @param **entry Pointer to store the entry point address to
 * @return Status code
 * @kinternal
 */
INTERNAL STATUS ExLoadProcessImage(const char *path, void (**entry)());

/**
 * @}
 */

#endif