#ifndef KERNEL_IO_VIDEO_OUTPUT_H_
#define KERNEL_IO_VIDEO_OUTPUT_H_

#include "defines.h"

EXPORT_API

#include "config.h"

struct IoDeviceObject;

/**
 * @brief Type of video output
 */
enum IoVideoType
{
    IO_VIDEO_UNKNOWN = 0, /**< Unknown video output type */
    IO_VIDEO_FRAME_BUFFER = 1,  /**< Linear frame buffer video */
};

/**
 * @brief Union of video output data structures
 */
union IoVideoOutput
{
    struct IoFrameBuffer fb; /**< Linear frame buffer */
};

/**
 * @brief Type of a callback for handling video output configuration change
 * @param handle Handle of a video output
 * @param *config New video output configuration. Must be copied by the callback
 * @param *context Context provided when registering this callback
 */
typedef void (*IoVideoConfigChangeHandler)(int handle, const union IoVideoOutput *config, void *context);

/**
 * @brief Get video output configuration
 * @param handle Video output handle
 * @param changeHandler Video config change callback
 * @param *context Change callback context
 * @param *output Video output configuration to be filled
 * @param *type Video output type to be filled
 * @return Status code
 * @note Priority level <= ::HAL_PRIORITY_LEVEL_DPC
 */
STATUS IoGetVideoOutput(int handle, IoVideoConfigChangeHandler changeHandler, void *context, union IoVideoOutput *const output, enum IoVideoType *const type);

/**
 * @brief Remove video output configuration change handler
 * @param handle Video output handle to match
 * @param changeHandler Video config change callback to match
 * @param *context Change callback context to match
 * @return Status code
 * @note Priority level <= ::HAL_PRIORITY_LEVEL_DPC
 */
STATUS IoRemoveVideoOutputConfigChangeHandler(int handle, IoVideoConfigChangeHandler changeHandler, void *context);

/**
 * @brief Register or update frame buffer
 * @param *dev Associated display device (device type = \a IO_DEVICE_TYPE_DISPLAY)
 * @param *config Frame buffer configuration. This structure is copied and stored by the kernel
 * @param address Physical frame buffer address
 * @return Status code
 * @note Priority level <= ::HAL_PRIORITY_LEVEL_DPC
 * @attention Physical frame buffer address and size must be aligned to at least 4 bytes
 */
STATUS IoRegisterFrameBuffer(const struct IoDeviceObject *dev, const struct IoFrameBufferConfig *config, PADDRESS address);

/**
 * @brief Draw video frame
 * @param handle Video output handle
 * @note Priority level <= ::HAL_PRIORITY_LEVEL_DPC
 */
void IoDrawVideo(int handle);

END_EXPORT_API

#endif