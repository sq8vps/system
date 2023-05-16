#ifndef KERNEL32_COMMON_H_
#define KERNEL32_COMMON_H_

#include <stdint.h>

/**
 * @brief Get string length
 * @param str Input string
 * @return String length (excluding null terminator)
*/
uint32_t Cm_strlen(const char *str);

/**
 * @brief Copy string
 * @param strTo Destination string
 * @param strFrom Source string
 * @return Destination string
*/
char* Cm_strcpy(char *strTo, const char *strFrom);

/**
 * @brief Compare strings
 * @param s1 1st string
 * @param s2 2nd string
 * @return 0 if identical
*/
int Cm_strcmp(const char *s1, const char *s2);

/**
 * @brief Copy memory
 * @param to Destination buffer
 * @param from Source buffer
 * @param n Number of bytes to copy
 * @return Destination buffer
*/
void* Cm_memcpy(void *to, const void *from, uintptr_t n);

#endif