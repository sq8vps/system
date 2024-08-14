#ifndef KERNEL_DEV_ENUMERATION_H_
#define KERNEL_DEV_ENUMERATION_H_

#include <stdint.h>
#include "defines.h"


struct IoDeviceNode;

/**
 * @brief Start device enumerating kernel thread
 * @return Status code
*/
INTERNAL STATUS IoStartDeviceEnumerationThread(void);

/**
 * @brief Notify device enumerator thread that a new device has been created
 * @param *node Device node
 * @return Status code
*/
INTERNAL STATUS IoNotifyDeviceEnumerator(struct IoDeviceNode *node);

#endif