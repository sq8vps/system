#ifndef FAT_UTILS_H_
#define FAT_UTILS_H_

#include <stddef.h>

/**
 * @brief Convert UCS-2 up to \a limit UTF-8 characters
 * @param *utf8 Output UTF-8 string
 * @param *ucs2 Input UCS-2 string
 * @param limit UTF-8 string size limit (bytes), excluding terminator
 * @return 0 on success, -1 on failure
 */
int FatUcs2ToUtf8(char *utf8, const char *ucs2, size_t limit);

/**
 * @brief Compare UCS-2 and UTF-8 strings
 * @param *ucs2 UCS-2 string
 * @param *utf8 UTF-8 string
 * @return 0 if equal, >0 if *ucs2>*utf8, <0 otherwise
 */
int FatCompareUcs2AndUtf8(const char *ucs2, const char *utf8);

/**
 * @brief Convert name to DOS name
 * @param *name Name to convert
 * @param *dosName Output DOS name, 12 bytes including terminator
 * @return 0 on success, <0 on failure
 */
int FatFileNameToDosName(const char *name, char *dosName);

/**
 * @brief Convert DOS name to UTF-8 name
 * @param *name Destination UTF-8 string
 * @param *dosName Source DOS 8.3 file name
 * @return 0 on success, <0 on failure
 */
int FatDosNameToFileName(char *name, const char *dosName);

#endif