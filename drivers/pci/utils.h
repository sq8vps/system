#ifndef PCI_UTILS_H_
#define PCI_UTILS_H_

#include <stdint.h>
#include "stdbool.h"
#include "class.h"
#include "defines.h"
#include "io/dev/rp.h"

uint32_t PciConfigReadDword(union IoBusId address, uint8_t offset);

uint16_t PciConfigReadWord(union IoBusId address, uint8_t offset);

uint8_t PciConfigReadByte(union IoBusId address, uint8_t offset);

uint16_t PciGetVendorId(union IoBusId address);

uint16_t PciGetDeviceId(union IoBusId address);

#define VENDOR_VALID(vendor) ((vendor) != 0xFFFF)
#define DEVICE_VALID(device) ((device) != 0xFFFF)

enum PciClass PciGetClass(union IoBusId address);

enum PciSubclass PciGetSubclass(union IoBusId address);

bool PciIsPciPciBridge(union IoBusId address);

bool PciIsHostBridge(union IoBusId address);

bool PciIsMultifunction(union IoBusId address);

STATUS PciReadConfigurationSpace(union IoBusId address, struct IoDriverRp *rp);

STATUS PciWriteConfigurationSpace(union IoBusId address, struct IoDriverRp *rp);

#endif