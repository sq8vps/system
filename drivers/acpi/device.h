#ifndef ACPI_DEVICE_H_
#define ACPI_DEVICE_H_

#include "acpica/include/acpi.h"
#include "io/dev/rp.h"

#define PCI_ADR_EXTRACT_DEVICE(adr) (((adr) >> 16) & 0xFFFF)
#define PCI_ADR_EXTRACT_FUNCTION(adr) ((adr) & 0xFFFF)

struct BusSubDeviceInfo
{
    char *path;
    enum IoBusType type;
    union IoBusId id;
    struct IoIrqMap *irqMap;
};

/**
 * @brief Get generic device name for given ACPI PNP ID
 * @param *id ACPI PNP ID
 * @return Pointer to name table
*/
char *AcpiGetPnpName(char *id);

ACPI_STATUS DriverGetBusInfoForDevice(struct IoDriverRp *rp);

ACPI_STATUS DriverEnumerate(struct ExDriverObject *drv);

/**
 * @brief Build IRQ mapping tree recursively for given PCI host bridge
 * @param device PCI host bridge ACPI object handle
 * @param **map Output map pointer (allocated by the function)
 * @return Status code
*/
ACPI_STATUS AcpiGetPciIrqTree(ACPI_HANDLE device, struct IoIrqMap **map);

/**
 * @brief Get IRQ mapping for given device
 * @param device ACPI device object handle
 * @param *info Device private data pointer
 * @return Status code
*/
ACPI_STATUS AcpiGetIrq(ACPI_HANDLE device, struct BusSubDeviceInfo *info);

#endif