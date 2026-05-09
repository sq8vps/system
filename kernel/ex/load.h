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

DRIVER_API
NABLA_API

/**
 * @brief Additional program data entry (for dynamic linker, interpreter, etc.)
 */
struct ExProgramData
{
    uint32_t type;
    union
    {
        uint64_t u64;
        uint32_t u32;
        size_t s;
        void *p;
    } value;
};

/**
 * @brief Types of program data entries
 */
enum ExProgramDataType
{
    PROGDATA_END = 0, /**< Final empty entry - array terminator */
    PROGDATA_BASE = 1, /**< Executable base address */
    PROGDATA_PAGE_SIZE = 2, /**< System page size */
};

END_NABLA_API

/**
 * @brief Get number of additional program data entries
 * @param *progData Program data entry array
 * @return Number of entries including terminator
 */
size_t ExGetProgramDataEntryCount(const struct ExProgramData *progData);

END_DRIVER_API

/**
 * @brief Load process image
 * @param *path Image path
 * @param **entry Pointer to store the entry point address to
 * @param **progData Place to return the additional program data array
 * @return Status code
 * @kinternal
 */
INTERNAL STATUS ExLoadProcessImage(const char *path, void (**entry)(void*), struct ExProgramData **progData);



/**
 * @}
 */

#endif