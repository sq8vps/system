#ifndef PCI_DEVICE_H_
#define PCI_DEVICE_H_

#include "defines.h"
#include "ex/kdrv.h"
#include "io/dev/dev.h"
#include "bridge.h"

struct PciDeviceData
{
    struct IoSubDeviceObject *enumerator;
    struct PciBridge *thisBridge;
};

STATUS PciEnumerate(struct ExDriverObject *drv, struct IoSubDeviceObject *dev, struct PciBridge *bridge);

#endif