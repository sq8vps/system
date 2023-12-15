#ifndef KERNEL_DEV_ENUMERATION_H_
#define KERNEL_DEV_ENUMERATION_H_

#include <stdint.h>
#include "defines.h"
#include "io/dev/dev.h"

/**
 * @brief Start device enumerating kernel thread
 * @return Status code
*/
INTERNAL STATUS IoStartDeviceEnumerationThread(void);

/**
 * @brief Notify device enumerator thread that a new device has been created
 * @param *dev Device object
 * @return Status code
*/
INTERNAL STATUS IoNotifyDeviceEnumerator(struct IoDeviceObject *dev);

#endif