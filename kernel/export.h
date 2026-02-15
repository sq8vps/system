#ifndef KERNEL_EXPORT_H_
#define KERNEL_EXPORT_H_

/**
 * @brief Mark all following lines up to the #END_DRIVER_API as driver API calls
 */
#define DRIVER_API

/**
 * @brief End to-be-exported block started with #DRIVER_API
 */
#define END_DRIVER_API

/**
 * @brief Mark all following lines up to the #END_NABLA_API as Native Nabla API calls
 */
#define NABLA_API

/**
 * @brief End to-be-exported block started with #NABLA_API
 */
#define END_NABLA_API

DRIVER_API
//dummy
END_DRIVER_API

#endif