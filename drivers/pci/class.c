#include "class.h"
#include <stdint.h>
#include <stddef.h>

#define PCI_NAME_INDEX(class, subclass) (((class) << 8) | (subclass))

static const char * const PciDeviceName[] = {
    [PCI_NAME_INDEX(STORAGE, STORAGE_SCSI)] = "Generic SCSI Controller",
    [PCI_NAME_INDEX(STORAGE, STORAGE_IDE)] = "Generic IDE Controller",
    [PCI_NAME_INDEX(STORAGE, STORAGE_FLOPPY)] = "Generic Floppy Controller",
    [PCI_NAME_INDEX(STORAGE, STORAGE_IPI)] = "Generic IPI Controller",
    [PCI_NAME_INDEX(STORAGE, STORAGE_RAID)] = "Generic RAID Controller",
    [PCI_NAME_INDEX(STORAGE, STORAGE_ATA)] = "Generic ATA Controller",
    [PCI_NAME_INDEX(STORAGE, STORAGE_SATA)] = "Generic SATA Controller",
    [PCI_NAME_INDEX(STORAGE, STORAGE_SAS)] = "Generic SAS Controller",
    [PCI_NAME_INDEX(STORAGE, STORAGE_NVME)] = "Generic NVMe Controller",
    [PCI_NAME_INDEX(STORAGE, STORAGE_OTHER)] = "Other Storage Controller",

    [PCI_NAME_INDEX(NETWORK, NETWORK_ETHERNET)] = "Generic Ethernet Controller",
    [PCI_NAME_INDEX(NETWORK, NETWORK_TOKEN_RING)] = "Generic Token Ring Controller",
    [PCI_NAME_INDEX(NETWORK, NETWORK_FDDI)] = "Generic FDDI Controller",
    [PCI_NAME_INDEX(NETWORK, NETWORK_ATM)] = "Generic ATM Controller",
    [PCI_NAME_INDEX(NETWORK, NETWORK_ISDN)] = "Generic ISDN Controller",
    [PCI_NAME_INDEX(NETWORK, NETWORK_OTHER)] = "Other Network Controller",

    [PCI_NAME_INDEX(DISPLAY, DISPLAY_VGA_8514)] = "Generic VGA Controller",
    [PCI_NAME_INDEX(DISPLAY, DISPLAY_XGA)] = "Generic XGA Controller",
    [PCI_NAME_INDEX(DISPLAY, DISPLAY_3D)] = "Generic 3D Controller",
    [PCI_NAME_INDEX(DISPLAY, DISPLAY_OTHER)] = "Other Display Controller",

    [PCI_NAME_INDEX(MULTIMEDIA, MULTIMEDIA_VIDEO)] = "Generic Video Controller",
    [PCI_NAME_INDEX(MULTIMEDIA, MULTIMEDIA_AUDIO)] = "Generic Audio Controller",
    [PCI_NAME_INDEX(MULTIMEDIA, MULTIMEDIA_TELEPHONE)] = "Generic Telephone Controller",
    [PCI_NAME_INDEX(MULTIMEDIA, MULTIMEDIA_OTHER)] = "Other Multimedia Controller",

    [PCI_NAME_INDEX(MEMORY, MEMORY_RAM)] = "Generic RAM Controller",
    [PCI_NAME_INDEX(MEMORY, MEMORY_FLASH)] = "Generic Flash Controller",
    [PCI_NAME_INDEX(MEMORY, MEMORY_OTHER)] = "Other Memory Controller",

    [PCI_NAME_INDEX(BRIDGE, BRIDGE_HOST)] = "Generic PCI Host Controller",
    [PCI_NAME_INDEX(BRIDGE, BRIDGE_ISA)] = "Generic PCI-ISA Bridge",
    [PCI_NAME_INDEX(BRIDGE, BRIDGE_EISA)] = "Generic PCI-EISA Bridge",
    [PCI_NAME_INDEX(BRIDGE, BRIDGE_MCA)] = "Generic PCI-MCA Bridge",
    [PCI_NAME_INDEX(BRIDGE, BRIDGE_PCI)] = "Generic PCI-PCI Bridge",
    [PCI_NAME_INDEX(BRIDGE, BRIDGE_PCMCIA)] = "Generic PCI-PCMCIA Bridge",
    [PCI_NAME_INDEX(BRIDGE, BRIDGE_NUBUS)] = "Generic PCI-NuBus Bridge",
    [PCI_NAME_INDEX(BRIDGE, BRIDGE_CARBUS)] = "Generic PCI-CardBus Bridge",
    [PCI_NAME_INDEX(BRIDGE, BRIDGE_RACEWAY)] = "Generic PCI-Raceway Bridge",
    [PCI_NAME_INDEX(BRIDGE, BRIDGE_PCI_2)] = "Generic PCI-PCI Bridge",
    [PCI_NAME_INDEX(BRIDGE, BRIDGE_OTHER)] = "Other Bridge Controller",

    [PCI_NAME_INDEX(SIMPLE_COMM, SIMPLE_COMM_SERIAL)] = "Generic Serial Controller",
    [PCI_NAME_INDEX(SIMPLE_COMM, SIMPLE_COMM_PARALLEL)] = "Generic Parallel Controller",
    [PCI_NAME_INDEX(SIMPLE_COMM, SIMPLE_COMM_MULTIPORT_SERIAL)] = "Generic Multiport Serial Controller",
    [PCI_NAME_INDEX(SIMPLE_COMM, SIMPLE_COMM_MODEM)] = "Generic Modem Controller",
    [PCI_NAME_INDEX(SIMPLE_COMM, SIMPLE_COMM_OTHER)] = "Other Communication Controller",

    [PCI_NAME_INDEX(SYSTEM_PERIPH, SYSTEM_PERIPH_PIC)] = "Generic PIC",
    [PCI_NAME_INDEX(SYSTEM_PERIPH, SYSTEM_PERIPH_DMA)] = "Generic DMA Controller",
    [PCI_NAME_INDEX(SYSTEM_PERIPH, SYSTEM_PERIPH_PIT)] = "Generic PIT",
    [PCI_NAME_INDEX(SYSTEM_PERIPH, SYSTEM_PERIPH_RTC)] = "Generic RTC",
    [PCI_NAME_INDEX(SYSTEM_PERIPH, SYSTEM_PERIPH_PCI_HOTPLUG)] = "Generic PCI Hotplug Controller",
    [PCI_NAME_INDEX(SYSTEM_PERIPH, SYSTEM_PERIPH_OTHER)] = "Other System Peripheral",

    [PCI_NAME_INDEX(INPUT_DEVICE, INPUT_DEVICE_KEYBOARD)] = "Generic Keyboard Controller",
    [PCI_NAME_INDEX(INPUT_DEVICE, INPUT_DEVICE_PEN)] = "Generic Pen Controller",
    [PCI_NAME_INDEX(INPUT_DEVICE, INPUT_DEVICE_MOUSE)] = "Generic Mouse Controller",
    [PCI_NAME_INDEX(INPUT_DEVICE, INPUT_DEVICE_SCANNER)] = "Generic Scanner Controller",
    [PCI_NAME_INDEX(INPUT_DEVICE, INPUT_DEVICE_GAMEPORT)] = "Generic Gameport Controller",
    [PCI_NAME_INDEX(INPUT_DEVICE, INPUT_DEVICE_OTHER)] = "Other Input Device",

    [PCI_NAME_INDEX(DOCKING_STATION, DOCKING_STATION_GENERIC)] = "Generic Docking Station Controller",
    [PCI_NAME_INDEX(DOCKING_STATION, DOCKING_STATION_OTHER)] = "Other Docking Station Controller",

    [PCI_NAME_INDEX(PROCESSOR, PROCESSOR_386)] = "Generic 386 CPU",
    [PCI_NAME_INDEX(PROCESSOR, PROCESSOR_486)] = "Generic 486 CPU",
    [PCI_NAME_INDEX(PROCESSOR, PROCESSOR_PENTIUM)] = "Generic Pentium CPU",
    [PCI_NAME_INDEX(PROCESSOR, PROCESSOR_ALPHA)] = "Generic Alpha CPU",
    [PCI_NAME_INDEX(PROCESSOR, PROCESSOR_POWERPC)] = "Generic PowerPC CPU",
    [PCI_NAME_INDEX(PROCESSOR, PROCESSOR_MIPS)] = "Generic MIPS CPU",
    [PCI_NAME_INDEX(PROCESSOR, PROCESSOR_COPROCESSOR)] = "Generic Coprocessor",

    [PCI_NAME_INDEX(SERIAL_BUS, SERIAL_BUS_1394)] = "Generic 1394 Controller",
    [PCI_NAME_INDEX(SERIAL_BUS, SERIAL_BUS_ACCESS)] = "Generic Access Controller",
    [PCI_NAME_INDEX(SERIAL_BUS, SERIAL_BUS_SSA)] = "Generic SSA Controller",
    [PCI_NAME_INDEX(SERIAL_BUS, SERIAL_BUS_USB)] = "Generic USB Controller",
    [PCI_NAME_INDEX(SERIAL_BUS, SERIAL_BUS_FIBRE)] = "Generic Fibre Controller",
    [PCI_NAME_INDEX(SERIAL_BUS, SERIAL_BUS_SMBUS)] = "Generic SMBus Controller",

    [PCI_NAME_INDEX(WIRELESS, WIRELESS_IRDA)] = "Generic IrDA Controller",
    [PCI_NAME_INDEX(WIRELESS, WIRELESS_IR)] = "Generic IR Controller",
    [PCI_NAME_INDEX(WIRELESS, WIRELESS_RF)] = "Generic RF Controller",
    [PCI_NAME_INDEX(WIRELESS, WIRELESS_OTHER)] = "Other Wireless Controller",

    [PCI_NAME_INDEX(INTELLIGENT, INTELLIGENT_IO)] = "Generic Intelligent IO Controller",

    [PCI_NAME_INDEX(SATELLITE_COMM, SATELLITE_COMM_TV)] = "Generic TV Controller",
    [PCI_NAME_INDEX(SATELLITE_COMM, SATELLITE_COMM_AUDIO)] = "Generic Audio Controller",
    [PCI_NAME_INDEX(SATELLITE_COMM, SATELLITE_COMM_VOICE)] = "Generic Voice Controller",
    [PCI_NAME_INDEX(SATELLITE_COMM, SATELLITE_COMM_DATA)] = "Generic Data Controller",

    [PCI_NAME_INDEX(ENCRYPTION, ENCRYPTION_NETWORK)] = "Generic Network Encryption Controller",
    [PCI_NAME_INDEX(ENCRYPTION, ENCRYPTION_ENTERTAINMENT)] = "Generic Entertainment Encryption Controller",
    [PCI_NAME_INDEX(ENCRYPTION, ENCRYPTION_OTHER)] = "Other Enctryption Controller",

    [PCI_NAME_INDEX(SIGNAL_PROCESSING, SIGNAL_PROCESSING_DPIO)] = "Generic DPIO Controller",
    [PCI_NAME_INDEX(SIGNAL_PROCESSING, SIGNAL_PROCESSING_OTHER)] = "Other Signal Processing Controller",

};

char *PciGetGenericDeviceName(enum PciClass class, enum PciSubclass subclass)
{
    uint16_t index = (class << 8) | subclass;
    if(index >= (sizeof(PciDeviceName) / sizeof(*PciDeviceName)))
        return NULL;
    
    return (char*)PciDeviceName[index];
}