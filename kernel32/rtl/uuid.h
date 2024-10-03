#ifndef RTL_UUID_H_
#define RTL_UUID_H_

#include <stdint.h>
#include "defines.h"

EXPORT_API

/**
 * @brief Length of an UUID string (constant)
 */
#define RTL_UUID_STRING_LENGTH sizeof("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx")

/**
 * @brief Change UUID endianess between LE and ME
 * @param *uid Input/output UUID pointer
 */
void RtlUuidConvertEndianess(void *uuid);

/**
 * @brief Convert UUID to string
 * @param *uid Input UUID, must be little endian (standard UUID)
 * @param *str Output hex UUID string pointer (length is always = \a RTL_UUID_STRING_LENGTH)
 * @param upperCase True for uppercase hex, false for lowercase hex
 */
void RtlUuidToString(const void *uuid, char *str, bool upperCase);

END_EXPORT_API

#endif