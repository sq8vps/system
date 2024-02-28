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
    uint16_t vendor;
    uint16_t device;
    enum PciClass class;
    enum PciSubclass subclass;
    struct PciBridge *thisBridge;
    union IoBusId address;
    struct IoIrqEntry irq;
    struct IoIrqMap *irqMap;
    struct
    {
        uint16_t irqAvailable : 1;
    };
    
};

/**
 * @brief Enumerate devices on a bus for given bridge
 * @param *drv Driver object
 * @param *dev This device object
 * @param *bridge PCI bridge object associated with this device
 * @return Status code 
*/
STATUS PciEnumerate(struct ExDriverObject *drv, struct IoDeviceObject *dev, struct PciBridge *bridge);

/**
 * @brief Create a Main Device Object (probably for PCI bridge)
 * @param *driverObject Driver object
 * @param *baseDeviceObject Base/Bus Device Object
 * @return Status code  
*/
STATUS PciAddDevice(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject);

/**
 * @brief Get Device ID for given device
 * @param *rp Request Packet that contains the request
 * @return Status code
*/
STATUS PciGetSystemDeviceId(struct IoRp *rp);

/**
 * @brief Get resources for given device
 * @param *rp Request Packet that contains the request
 * @return Status code
*/
STATUS PciGetResources(struct IoRp *rp);


#endif