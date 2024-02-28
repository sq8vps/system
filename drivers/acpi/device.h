#ifndef ACPI_DEVICE_H_
#define ACPI_DEVICE_H_

#include "acpica/include/acpi.h"
#include "io/dev/rp.h"
#include "io/dev/dev.h"

#define PCI_ADR_EXTRACT_DEVICE(adr) (((adr) >> 16) & 0xFFFF)
#define PCI_ADR_EXTRACT_FUNCTION(adr) ((adr) & 0xFFFF)

#define ACPI_PNP_ID_MAX_LENGTH 8

#define ACPI_IS_PCI_HOST_BRIDGE(pnpId) !CmStrcmp((pnpId), "PNP0A03") || !CmStrcmp((pnpId), "PNP0A08") 

struct AcpiDeviceInfo
{
    char pnpId[ACPI_PNP_ID_MAX_LENGTH + 1]; /**< ACPI or PNP ID */
    enum IoBusType type; /**< Bus type this controller handles */
    union IoBusId id; /**< Bus-specific ID of this controller */
    
    uint32_t resourceCount; /**< Number of associated resources */
    struct IoDeviceResource *resource; /**< Resource table */
    char path[]; /**< ACPI path */
};

/**
 * @brief Get generic device name for given ACPI PNP ID
 * @param *id ACPI PNP ID
 * @return Pointer to name table
*/
char *AcpiGetPnpName(char *id);

/**
 * @brief Get device location data
 * @param *rp Input/output RP
 * @return Status code
*/
STATUS AcpiGetDeviceLocation(struct IoRp *rp);

STATUS AcpiGetDeviceResources(struct IoRp *rp);

ACPI_STATUS DriverEnumerate(struct ExDriverObject *drv, struct IoDeviceObject *dev);

ACPI_STATUS AcpiFillResourceList(ACPI_HANDLE device, struct AcpiDeviceInfo *info);

STATUS AcpiGetDeviceId(struct IoRp *rp);

#endif