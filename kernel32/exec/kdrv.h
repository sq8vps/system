#ifndef KERNEL32_KDRV_H_
#define KERNEL32_KDRV_H_

/**
 * @file kdrv.h
 * @brief Kernel mode driver library
 * 
 * Provides kernel mode drivers manipulation routines
*/

#include <stdint.h>
#include "../defines.h"
#include "../ddk/ddk.h"


/**
 * @defgroup kdrvLoad Kernel mode driver loading
 * @ingroup exec
 * @{
*/

/**
 * @brief Relocate, link and execute preloaded driver
 * @param driverImage Raw driver image address
 * @param size Bytes occupied by the driver
 * @return Error code
 * @attention Full raw driver image must be preloaded to memory. All no-bits/BSS sections must be allocated before.
*/
kError_t Ex_loadPreloadedDriver(const uintptr_t driverImage, const uintptr_t size);


/**
 * @}
*/

/**
 * @defgroup kdrvCb Kernel mode driver callbacks handling
 * @ingroup exec
 * @{
*/



/**
 * @brief Register general driver callbacks
 * @param index Driver index
 * @param callbacks Structure of callbacks
 * @return Error code
*/
extern kError_t Ex_registerDriverGeneralCallbacks(const KDRV_INDEX_T index, const DDK_KDrvGeneralCallbacks_t *callbacks);

/**
 * @brief (Un)Register specialized driver callbacks
 * @param index Driver index
 * @param callbacks Structure of callbacks. NULL to unregister
 * @return Error code
 * @warning Callback structure is determined by driver class
*/
extern kError_t Ex_registerDriverCallbacks(const KDRV_INDEX_T index, const void *callbacks);

/**
 * @}
*/

#endif