#ifndef ACPI_DEVICE_H_
#define ACPI_DEVICE_H_

#include "acpica/include/acpi.h"

struct BusSubDeviceInfo
{
    char *path;
    union
    {
        struct
        {
            uint8_t bus;
            uint8_t device;
            uint8_t function;
        } pci;
    } id;
};

ACPI_STATUS DriverGetBusInfoForDevice(struct IoSubDeviceObject *device, struct BusSubDeviceInfo **info);

ACPI_STATUS DriverEnumerate(struct ExDriverObject *drv);

#endif