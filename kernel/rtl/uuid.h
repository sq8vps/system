/**
 * @file uuid.h
 * @brief UUID support
 * @ingroup rtl_uuid
 */

#ifndef RTL_UUID_H_
#define RTL_UUID_H_

#include <stdint.h>
#include "defines.h"

/**
 * @addtogroup rtl_uuid UUID support
 * @ingroup rtl
 * @{
 */

EXPORT_API

/**
 * @brief Length of an UUID string (constant)
 */
#define RTL_UUID_STRING_LENGTH sizeof("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx")

/**
 * @brief Change UUID endianess between LE and ME
 * @param *uid Input/output UUID pointer (consecutive 16 bytes, 1 byte aligned)
 */
void RtlUuidConvertEndianess(void *uuid);

/**
 * @brief Convert UUID to string
 * @param *uid Input UUID, must be little endian (standard UUID, consecutive 16 bytes, 1 byte aligned)
 * @param *str Output hex UUID string pointer (length is always = #RTL_UUID_STRING_LENGTH)
 * @param upperCase True for uppercase hex, false for lowercase hex
 */
void RtlUuidToString(const void *uuid, char *str, bool upperCase);

END_EXPORT_API

/**
 * @}
 */

#endif