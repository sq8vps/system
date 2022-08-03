/*
 * common.h
 *
 *  Created on: 17.04.2020
 *      Author: Piotr
 */

#ifndef KERNEL_COMMON_H_
#define KERNEL_COMMON_H_

#include <stdint.h>

#define PAGE_SIZE 4096

/**
 * \brief Copies num bytes from *from to *to
 * \param[out] *to Destination pointer
 * \param[in] *from Source pointer
 * \param[in] num Number of bytes to copy
 * \return Returns 'to' argument
 */
void *memcpy(void *to, const void *from, uint64_t num);

/**
 * \brief Aligns selected address to the nearest possible page start address (always rounding up)
 * \param[in] addr Address to be aligned
 * \return Address aligned to PAGE_SIZE
 */
uint64_t alignToPage(uint64_t addr);


/**
 * \brief Fast 10^x power
 * \param x Exponent - integer 0-255
 * \return 10^x
 */
uint64_t pow10(uint8_t x);


/**
 * \brief Convert 4 little-endian bytes to uint32
 * \param in Input bytes
 * \return uint32 byte
 */
uint32_t bytesToUint32LE(uint8_t *in);


/**
 * \brief Convert 4 little-endian bytes to uint32
 * \param in Input bytes
 * \return uint32 byte
 */
uint16_t bytesToUint16LE(uint8_t *in);


uint32_t strlen(const uint8_t *s);

uint8_t strcmp(const uint8_t *s1, const uint8_t *s2);

uint8_t strncmp(const uint8_t *s1, const uint8_t *s2, uint32_t len);

#endif /* KERNEL_COMMON_H_ */
