#ifndef KERNEL_ORDER_H_
#define KERNEL_ORDER_H_

#include <stdint.h>
#include "defines.h"

/**
 * @brief Initialize endianness converter
*/
INTERNAL void CmDetectEndianness(void);

EXPORT_API

/**
 * @brief Convert CPU <-> LE endiannes of uint16_t
 * @param x Value to convert
 * @return Converted value
*/
uint16_t CmLeU16(uint16_t x);


/**
 * @brief Convert CPU <-> LE endiannes of int16_t
 * @param x Value to convert
 * @return Converted value
*/
int16_t CmLeS16(int16_t x);


/**
 * @brief Convert CPU <-> LE endiannes of uint32_t
 * @param x Value to convert
 * @return Converted value
*/
uint32_t CmLeU32(uint32_t x);


/**
 * @brief Convert CPU <-> LE endiannes of int32_t
 * @param x Value to convert
 * @return Converted value
*/
int32_t CmLeS32(int32_t x);


/**
 * @brief Convert CPU <-> LE endiannes of uint64_t
 * @param x Value to convert
 * @return Converted value
*/
uint64_t CmLeU64(uint64_t x);


/**
 * @brief Convert CPU <-> LE endiannes of int64_t
 * @param x Value to convert
 * @return Converted value
*/
int64_t CmLeS64(int64_t x);


/**
 * @brief Convert CPU <-> BE endiannes of uint16_t
 * @param x Value to convert
 * @return Converted value
*/
uint16_t CmBeU16(uint16_t x);


/**
 * @brief Convert CPU <-> BE endiannes of int16_t
 * @param x Value to convert
 * @return Converted value
*/
int16_t CmBeS16(int16_t x);


/**
 * @brief Convert CPU <-> BE endiannes of uint32_t
 * @param x Value to convert
 * @return Converted value
*/
uint32_t CmBeU32(uint32_t x);


/**
 * @brief Convert CPU <-> BE endiannes of int32_t
 * @param x Value to convert
 * @return Converted value
*/
int32_t CmBeS32(int32_t x);


/**
 * @brief Convert CPU <-> BE endiannes of uint64_t
 * @param x Value to convert
 * @return Converted value
*/
uint64_t CmBeU64(uint64_t x);


/**
 * @brief Convert CPU <-> BE endiannes of int64_t
 * @param x Value to convert
 * @return Converted value
*/
int64_t CmBeS64(int64_t x);

END_EXPORT_API

#endif