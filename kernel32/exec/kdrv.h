#ifndef KERNEL32_KDRV_H_
#define KERNEL32_KDRV_H_

#include <stdint.h>
#include "../defines.h"

/**
 * @brief Relocate, link and execute preloaded driver
 * @param driverImage Raw driver image address
 * @param size Bytes occupied by the driver
 * @return Error code
 * @attention Full raw driver image must be preloaded to memory. All no-bits/BSS sections must be allocated before.
*/
kError_t Ex_loadPreloadedDriver(uintptr_t *driverImage, uintptr_t size);

kError_t Ex_registerGeneralDriverCallback();

#endif