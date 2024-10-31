
#include <stdint.h>
#include <stddef.h>
#include "rtl/string.h"

static const char *const AcpiPnpNames[] = {
    [0x0000] = "AT Interrupt Controller",
    [0x0001] = "EISA Interrupt Controller",
    [0x0002] = "MCA Interrupt Controller",
    [0x0003] = "APIC",
    [0x0100] = "AT Timer",
    [0x0101] = "EISA Timer",
    [0x0102] = "MCA Timer",
    [0x0103] = "HPET",
    [0x0200] = "AT DMA Controller",
    [0x0201] = "EISA DMA Controller",
    [0x0202] = "MCA DMA Controller",
    [0x0400] = "Standard LPT printer port",
    [0x0500] = "Standard PC COM port",
    [0x0A00] = "ISA Bus",
    [0x0A01] = "EISA Bus",
    [0x0A02] = "MCA Bus",
    [0x0A03] = "PCI Bus",
    [0x0A04] = "VESA/VL Bus",
    [0x0A05] = "Generic ACPI Bus",
    [0x0A06] = "Generic ACPI Extended-IO Bus",
    [0x0A08] = "PCI-Express Bus",
    [0x0C08] = "ACPI system board hardware",
    [0x0C09] = "ACPI Embedded Controller",
    [0x0C0A] = "ACPI Control Method Battery",
    [0x0C0B] = "ACPI Fan",
    [0x0C0C] = "ACPI power button device",
    [0x0C0D] = "ACPI lid device",
    [0x0C0E] = "ACPI sleep button device",
    [0x0C10] = "ACPI system indicator device",
    [0x0C11] = "ACPI thermal zone",
    [0x0C12] = "Device Bay Controller",
};

static uint32_t AcpiExtractHex(const char *const hex)
{
    uint32_t value = 0;
    for(uint16_t i = 0; i < RtlStrlen(hex); i++)
    {
        uint8_t v;
        if((hex[i] >= '0') && (hex[i] <= '9'))
            v = hex[i] - '0';
        else if((hex[i] >= 'A') && (hex[i] <= 'F'))
            v = hex[i] - 'A';
        else
            return 0;
        value <<= 4;
        value |= v;
    }
    return value;
}

char *AcpiGetPnpName(const char *const id)
{
    if(!RtlStrncmp(id, "PNP", 3))
    {
        uint16_t index = AcpiExtractHex(&(id[3]));
        if(index >= (sizeof(AcpiPnpNames) / sizeof(*AcpiPnpNames)))
            return NULL;
        return (char*)AcpiPnpNames[index];
    }
    else
        return NULL;
}