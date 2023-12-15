#ifndef PCI_UTILS_H_
#define PCI_UTILS_H_

#include <stdint.h>
#include "stdbool.h"
#include "class.h"
#include "defines.h"
#include "io/dev/rp.h"

struct PciAddress
{
    uint16_t bus : 8;
    uint16_t device : 5;
    uint16_t function : 3;
};

uint32_t PciConfigReadDword(struct PciAddress address, uint8_t offset);

uint16_t PciConfigReadWord(struct PciAddress address, uint8_t offset);

uint8_t PciConfigReadByte(struct PciAddress address, uint8_t offset);

uint16_t PciGetVendorId(struct PciAddress address);

uint16_t PciGetDeviceId(struct PciAddress address);

#define VENDOR_VALID(vendor) ((vendor) != 0xFFFF)
#define DEVICE_VALID(device) ((device) != 0xFFFF)

enum PciClass PciGetClass(struct PciAddress address);

enum PciSubclass PciGetSubclass(struct PciAddress address);

bool PciIsPciPciBridge(struct PciAddress address);

bool PciIsHostBridge(struct PciAddress address);

bool PciIsMultifunction(struct PciAddress address);

STATUS PciReadConfigurationSpace(struct PciAddress address, struct IoDriverRp *rp);

STATUS PciWriteConfigurationSpace(struct PciAddress address, struct IoDriverRp *rp);

#endif