#ifndef PCI_DEVICE_H_
#define PCI_DEVICE_H_

#include "defines.h"
#include "ex/kdrv.h"
#include "hal/interrupt.h"
#include "io/dev/dev.h"
#include "io/dev/rp.h"
#include "bridge.h"

struct PciDeviceData
{
    struct PciBridge *thisBridge;
    union IoBusId address;
    struct IoIrqMap *irqMap;
    struct IoIrqEntry irq;
};

/**
 * @brief Enumerate devices on a bus for given bridge
 * @param *drv Driver object
 * @param *dev This device object
 * @param *bridge PCI bridge object associated with this device
 * @return Status code 
*/
STATUS PciEnumerate(struct ExDriverObject *drv, struct IoSubDeviceObject *dev, struct PciBridge *bridge);

/**
 * @brief Perform a subdevice creation and form a device stack
 * @param *driverObject Driver object
 * @param *baseDeviceObject Base subdevice object, that is a base subdevice object for this device object
 * @return Status code  
*/
STATUS PciAddDevice(struct ExDriverObject *driverObject, struct IoSubDeviceObject *baseDeviceObject);

#endif