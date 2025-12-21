#ifndef KERNEL_RTL_CRC_H
#define KERNEL_RTL_CRC_H

#include <stdint.h>

/**
 * @brief Compute the CRC32 checksum of the given data
 * @param initial Initial CRC value
 * @param polynomial Polynomial to use
 * @param *data Data pointer
 * @param size Size of the data
 * @return Computed CRC32 checksum
 */
uint32_t RtlCrc32(uint32_t initial, uint32_t polynomial, const void *data, uint32_t size);

#endif