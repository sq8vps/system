#ifndef KERNEL_IO_VIDEO_CONFIG_H_
#define KERNEL_IO_VIDEO_CONFIG_H_

#include "defines.h"

EXPORT_API

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
        } red, green, blue, reserved; /**< Red, green, blue, and reserved bit masks */
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

END_EXPORT_API

#endif