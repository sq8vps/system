#ifndef KERNEL_INPUT_H_
#define KERNEL_INPUT_H_

#include "defines.h"
#include <stdint.h>

EXPORT_API

struct IoDeviceObject;
struct IoEventHandler;
union IoEventData;

/**
 * @brief Register new input device
 * @param *dev Device object
 * @param *handle Assigned input device handle number
 * @return Status code
 * @note Priority level <= HAL_PRIORITY_LEVEL_DPC
 */
STATUS IoRegisterInputDevice(const struct IoDeviceObject *dev, int *handle);

/**
 * @brief Register event handler
 * @param *handler Event handler structure
 * @return Status code
 * @note Priority level <= HAL_PRIORITY_LEVEL_DPC
 */
STATUS IoRegisterEventHandler(const struct IoEventHandler *handler);

/**
 * @brief Report event and notify recipients
 * @param handle Event handle number
 * @param *data Event data
 * @return Status code
 * @note Priority level <= HAL_PRIORITY_LEVEL_DPC
 */
STATUS IoReportEvent(int handle, const union IoEventData *data);

END_EXPORT_API

#endif