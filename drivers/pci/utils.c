#include "utils.h"
#include "kernel.h"

#define PCI_CONFIG_IO_ENABLE_FLAG 0x80000000
#define PCI_CONFIG_IO_ADDR 0xCF8
#define PCI_CONFIG_IO_DATA 0xCFC

#define PCI_CONFIG_STD_VENDOR_ID 0x00
#define PCI_CONFIG_STD_DEVICE_ID 0x02
#define PCI_CONFIG_STD_COMMAND 0x04
#define PCI_CONFIG_STD_STATUS 0x06
#define PCI_CONFIG_STD_REVISION_ID 0x08
#define PCI_CONFIG_STD_PROG_IF 0x09
#define PCI_CONFIG_STD_SUBCLASS 0x0A
#define PCI_CONFIG_STD_CLASS 0x0B
#define PCI_CONFIG_STD_CACHE_LINE_SIZE 0x0C
#define PCI_CONFIG_STD_LATENCY_TIMER 0x0D
#define PCI_CONFIG_STD_HEADER_TYPE 0x0E
#define PCI_CONFIG_STD_BIST 0x0F
#define PCI_CONFIG_STD_BAR0 0x10
#define PCI_CONFIG_STD_BAR1 0x14
#define PCI_CONFIG_STD_BAR2 0x18
#define PCI_CONFIG_STD_BAR3 0x1C
#define PCI_CONFIG_STD_BAR4 0x20
#define PCI_CONFIG_STD_BAR5 0x24

#define PCI_CONFIG_HEADER_MULTIFUNCTION 0x80

uint32_t PciConfigReadDword(union IoBusId address, uint8_t offset)
{
	uint32_t data = (((uint32_t)address.pci.bus << 16) | ((uint32_t)address.pci.device << 11) |
     ((uint32_t)address.pci.function << 8) | (offset & 0xFC) | PCI_CONFIG_IO_ENABLE_FLAG);
	HalIoPortWriteDWord(PCI_CONFIG_IO_ADDR, data);
	return HalIoPortReadDWord(PCI_CONFIG_IO_DATA);
}

uint16_t PciConfigReadWord(union IoBusId address, uint8_t offset)
{
	uint32_t data = (((uint32_t)address.pci.bus << 16) | ((uint32_t)address.pci.device << 11) |
     ((uint32_t)address.pci.function << 8) | (offset & 0xFC) | PCI_CONFIG_IO_ENABLE_FLAG);
	HalIoPortWriteDWord(PCI_CONFIG_IO_ADDR, data);
	return (HalIoPortReadDWord(PCI_CONFIG_IO_DATA) >> ((offset & 2) << 3)) & 0xFFFF;
}

uint8_t PciConfigReadByte(union IoBusId address, uint8_t offset)
{
	uint32_t data = (((uint32_t)address.pci.bus << 16) | ((uint32_t)address.pci.device << 11) |
     ((uint32_t)address.pci.function << 8) | (offset & 0xFC) | PCI_CONFIG_IO_ENABLE_FLAG);
	HalIoPortWriteDWord(PCI_CONFIG_IO_ADDR, data);
	return (HalIoPortReadDWord(PCI_CONFIG_IO_DATA) >> ((offset & 3) << 3)) & 0xFF;
}

void PciConfigWriteDword(union IoBusId address, uint8_t offset, uint32_t data)
{
	uint32_t a = (((uint32_t)address.pci.bus << 16) | ((uint32_t)address.pci.device << 11) |
     ((uint32_t)address.pci.function << 8) | (offset & 0xFC) | PCI_CONFIG_IO_ENABLE_FLAG);
	HalIoPortWriteDWord(PCI_CONFIG_IO_ADDR, a);
	HalIoPortWriteDWord(PCI_CONFIG_IO_DATA, data);
}

void PciConfigWriteByte(union IoBusId address, uint8_t offset, uint8_t data)
{
	uint32_t old = PciConfigReadDword(address, offset & 0xFC) & ~((uint32_t)0xFF << ((offset & 0x3) * 8));
	PciConfigWriteDword(address, offset & 0xFC, old | (data << ((offset & 0x03) * 8)));
}

uint16_t PciGetVendorId(union IoBusId address)
{
    return PciConfigReadWord(address, PCI_CONFIG_STD_VENDOR_ID);
}

uint16_t PciGetDeviceId(union IoBusId address)
{
    return PciConfigReadWord(address, PCI_CONFIG_STD_DEVICE_ID);
}

enum PciClass PciGetClass(union IoBusId address)
{
	return PciConfigReadByte(address, PCI_CONFIG_STD_CLASS);
}

enum PciSubclass PciGetSubclass(union IoBusId address)
{
	return PciConfigReadByte(address, PCI_CONFIG_STD_SUBCLASS);
}

bool PciIsMultifunction(union IoBusId address)
{
	return PciConfigReadByte(address, PCI_CONFIG_STD_HEADER_TYPE) & PCI_CONFIG_HEADER_MULTIFUNCTION;
}

bool PciIsPciPciBridge(union IoBusId address)
{
	if(BRIDGE == PciGetClass(address))
	{
		if((BRIDGE_PCI == PciGetSubclass(address)) || (BRIDGE_PCI_2 == PciGetSubclass(address)))
			return true;
	}
	return false;
}

bool PciIsHostBridge(union IoBusId address)
{
	if(BRIDGE == PciGetClass(address))
	{
		if(BRIDGE_HOST == PciGetSubclass(address))
			return true;
	}
	return false;
}

STATUS PciReadConfigurationSpace(union IoBusId address, struct IoDriverRp *rp)
{
	if(0 == rp->size)
		return OK;
	if(NULL == (rp->buffer = MmAllocateKernelHeap(rp->size)))
	{
		return OUT_OF_RESOURCES;
	}
	uint8_t *d = (uint8_t*)rp->buffer;
	for(uint64_t i = 0; i < rp->size; i++)
	{
		d[i] = PciConfigReadByte(address, i);
	}
	return OK;
}

STATUS PciWriteConfigurationSpace(union IoBusId address, struct IoDriverRp *rp)
{
	if(0 == rp->size)
		return OK;
	if(NULL == rp->buffer)
		return NULL_POINTER_GIVEN;
	uint8_t *d = (uint8_t*)rp->buffer;
	for(uint64_t i = 0; i < rp->size; i++)
	{
		PciConfigWriteByte(address, i, d[i]);
	}
	return OK;
}