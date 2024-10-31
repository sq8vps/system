#ifndef RTL_ORDER_H_
#define RTL_ORDER_H_

#include <stdint.h>
#include "defines.h"

/**
 * @brief Initialize endianness converter
*/
INTERNAL void RtlDetectEndianness(void);

EXPORT_API

/**
 * @brief Convert CPU <-> LE endiannes of uint16_t
 * @param x Value to convert
 * @return Converted value
*/
uint16_t RtlLeU16(uint16_t x);


/**
 * @brief Convert CPU <-> LE endiannes of int16_t
 * @param x Value to convert
 * @return Converted value
*/
int16_t RtlLeS16(int16_t x);


/**
 * @brief Convert CPU <-> LE endiannes of uint32_t
 * @param x Value to convert
 * @return Converted value
*/
uint32_t RtlLeU32(uint32_t x);


/**
 * @brief Convert CPU <-> LE endiannes of int32_t
 * @param x Value to convert
 * @return Converted value
*/
int32_t RtlLeS32(int32_t x);


/**
 * @brief Convert CPU <-> LE endiannes of uint64_t
 * @param x Value to convert
 * @return Converted value
*/
uint64_t RtlLeU64(uint64_t x);


/**
 * @brief Convert CPU <-> LE endiannes of int64_t
 * @param x Value to convert
 * @return Converted value
*/
int64_t RtlLeS64(int64_t x);


/**
 * @brief Convert CPU <-> BE endiannes of uint16_t
 * @param x Value to convert
 * @return Converted value
*/
uint16_t RtlBeU16(uint16_t x);


/**
 * @brief Convert CPU <-> BE endiannes of int16_t
 * @param x Value to convert
 * @return Converted value
*/
int16_t RtlBeS16(int16_t x);


/**
 * @brief Convert CPU <-> BE endiannes of uint32_t
 * @param x Value to convert
 * @return Converted value
*/
uint32_t RtlBeU32(uint32_t x);


/**
 * @brief Convert CPU <-> BE endiannes of int32_t
 * @param x Value to convert
 * @return Converted value
*/
int32_t RtlBeS32(int32_t x);


/**
 * @brief Convert CPU <-> BE endiannes of uint64_t
 * @param x Value to convert
 * @return Converted value
*/
uint64_t RtlBeU64(uint64_t x);


/**
 * @brief Convert CPU <-> BE endiannes of int64_t
 * @param x Value to convert
 * @return Converted value
*/
int64_t RtlBeS64(int64_t x);

END_EXPORT_API

#endif