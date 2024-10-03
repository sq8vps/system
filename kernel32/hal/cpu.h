#ifndef KERNEL_HAL_CPU_H_
#define KERNEL_HAL_CPU_H_

#include <stdint.h>
#include "defines.h"
#include "config.h"
#include "archdefs.h"

EXPORT_API

/**
 * @brief A general bitmap CPU representation
 */
typedef struct 
{
    uint32_t u32[CEIL_DIV(MAX_CPU_COUNT, sizeof(uint32_t) * 8)];
} HalCpuBitmap;

/**
 * @brief A constant representing all CPUs in \a HalCpuBitmap
 */
#define HAL_CPU_ALL (HalCpuBitmap){.u32[0 ... CEIL_DIV(MAX_CPU_COUNT, sizeof(uint32_t) * 8) - 1] = UINT32_MAX}

/**
 * @brief A constant representing no CPUs in \a HalCpuBitmap
 */
#define HAL_CPU_NONE (HalCpuBitmap){.u32[0 ... CEIL_DIV(MAX_CPU_COUNT, sizeof(uint32_t) * 8) - 1] = 0}

/**
 * @brief Get number of bits set in \a HalCpuBitmap
 * @param bitmap CPU bitmap
 * @param count Variable to store the count to
 */
#define HAL_GET_CPU_BIT_COUNT(bitmap, count) do { \
    (count) = 0; \
    for(uint16_t HAL_GET_CPU_BIT_COUNT_i = 0; HAL_GET_CPU_BIT_COUNT_i < CEIL_DIV(MAX_CPU_COUNT, sizeof(uint32_t) * 8); HAL_GET_CPU_BIT_COUNT_i++) \
        (count) += __builtin_popcount(bitmap->u32[HAL_GET_CPU_BIT_COUNT_i]); \
    } while(0);

/**
 * @brief Get CPU bit from \a HalCpuBitmap type variable
 * @param bitmap CPU bitmap of type \a HalCpuBitmap
 * @param cpu CPU number
 * @return Bit state for given CPU
 */
#define HAL_GET_CPU_BIT(bitmap, cpu) (!!((bitmap)->u32[(cpu) >> 5] & ((uint32_t)1 << ((cpu) & 0x1F)))) 

/**
 * @brief Set CPU bit in \a HalCpuBitmap type variable
 * @param bitmap CPU bitmap of type \a HalCpuBitmap
 * @param cpu CPU number 
 */
#define HAL_SET_CPU_BIT(bitmap, cpu) ((bitmap)->u32[(cpu) >> 5] |= ((uint32_t)1 << ((cpu) & 0x1F))) 

/**
 * @brief Clear CPU bit in \a HalCpuBitmap type variable
 * @param bitmap CPU bitmap of type \a HalCpuBitmap
 * @param cpu CPU number 
 */
#define HAL_CLEAR_CPU_BIT(bitmap, cpu) ((bitmap)->u32[(cpu) >> 5] &= ~((uint32_t)1 << ((cpu) & 0x1F))) 

/**
 * @brief General CPU structure
 */
struct HalCpu
{
    uint32_t number; /**< CPU number (by registration order) */
    bool usable; /**< Is CPU usable by the system? */
    struct HalCpuExtensions extensions; /**< Architecture-specific extensions */
};

/**
 * @brief Get CPU entry associated with given ID
 * @param id CPU ID
 * @return CPU entry pointer or NULL on failure
 */
struct HalCpu* HalGetCpuEntry(uint32_t id);

/**
 * @brief Get number of all (usable and unusable) CPUs
 * @return Number of all CPUs
 */
uint32_t HalGetCpuCount(void);

END_EXPORT_API

/**
 * @brief Register new CPU in kernel
 * @param *extensions Pointer to architecture-specific CPU extensions to be copied
 * @param usable True if CPU usable, false otherwise
 * @return Status code
 */
INTERNAL STATUS HalRegisterCpu(struct HalCpuExtensions *extensions, bool usable);

#endif