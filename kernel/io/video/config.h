/**
 * @file config.h
 * @brief Video adapter abstraction layer definitions
 * @ingroup io_video
 */

#ifndef KERNEL_IO_VIDEO_CONFIG_H_
#define KERNEL_IO_VIDEO_CONFIG_H_

#include "defines.h"

/**
 * @addtogroup io_video Video adapter support layer
 * @brief Video adapter support layer
 * @ingroup io
 * @{
 */

DRIVER_API

/**
 * @brief Color mask indices for \ref IoFrameBufferConfig
 */
enum IoColorMaskIndex
{
    RED_INDEX = 0,
    GREEN_INDEX = 1,
    BLUE_INDEX = 2,
    RESERVED_INDEX = 3,
};

/**
 * @brief General frame buffer configuration data
 */
struct IoFrameBufferConfig
{
    uint32_t width; /**< Width (X resolution) in pixels */
    uint32_t height; /**< Height (Y resolution) in pixels */
    uint32_t pitch; /**< Bytes per line */
    uint8_t bitsPerPixel; /**< Bits per pixel */
    struct
    {
        struct
        {
            uint8_t size; /**< Color mask size in bits */
            uint8_t position; /**< Mask position (count of left shifts) */
        } color[4]; /**< Bit masks for colors. Use values from \ref IoColorMaskIndex for addresing. */
    } mask; /**< Color masks */
};

/**
 * @brief Frame buffer data for application
 */
struct IoFrameBuffer
{
    struct IoFrameBufferConfig config; /**< Frame buffer config */
    void *fb; /**< Frame buffer pointer */
};

END_DRIVER_API

/**
 * @}
 */

#endif