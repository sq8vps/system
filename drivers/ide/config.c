#include "device.h"
#include "logging.h"



#define PCI_IDE_DEFAULT_PRIMARY_COMMAND_PORT 0x1F0
#define PCI_IDE_DEFAULT_PRIMARY_CONTROL_PORT 0x3F6
#define PCI_IDE_DEFAULT_SECONDARY_COMMAND_PORT 0x170
#define PCI_IDE_DEFAULT_SECONDARY_CONTROL_PORT 0x376

#define PCI_IDE_CLASS_CODE 0x01
#define PCI_IDE_SUBCLASS_CODE 0x01

#define PCI_IDE_PROGIF_OPMODE_PRIMARY_NATIVE 0x01
#define PCI_IDE_PROGIF_OPMODE_SECONDARY_NATIVE 0x04
#define PCI_IDE_PROGIF_BUS_MASTER_CAPABLE 0x80

#define PCI_IDE_COMMAND_PRIMARY_BAR 0x00
#define PCI_IDE_CONTROL_PRIMARY_BAR 0x01
#define PCI_IDE_COMMAND_SECONDARY_BAR 0x02
#define PCI_IDE_CONTROL_SECONDARY_BAR 0x03
#define PCI_IDE_BUS_MASTER_BAR 0x04

#define PCI_IDE_BAR_IO_PORT_FLAG 0x01
#define PCI_IDE_BAR_COMMAND_IO_PORT_MASK 0xFFF8
#define PCI_IDE_BAR_CONTROL_IO_PORT_MASK 0xFFFC
#define PCI_IDE_BAR_BUS_MASTER_IO_PORT_MASK 0xFFF8



STATUS IdeConfigureController(struct IoSubDeviceObject *device, struct IdeDeviceData *info)
{
    STATUS status = OK;
    
    struct IoDriverRp *rp;
    if(OK != (status = IoCreateRp(&rp)))
        return status;
    
    rp->code = IO_RP_GET_DEVICE_CONFIGURATION;
    rp->payload.deviceConfiguration.type = IO_BUS_TYPE_PCI;
    rp->payload.deviceConfiguration.device = device;
    rp->flags = IO_DRIVER_RP_FLAG_SYNCHRONOUS;
    rp->size = sizeof(struct IoPciDeviceHeader);
    rp->buffer = NULL;

    if(OK != (status = IoSendRp(device->attachedTo, NULL, rp)))
    {
        LOG(SYSLOG_ERROR, "Getting PCI configuration failed with status 0x%X\n", status);
        MmFreeKernelHeap(rp->buffer);
        MmFreeKernelHeap(rp);
        return status;
    }
    struct IoPciDeviceHeader *hdr = rp->buffer;
    MmFreeKernelHeap(rp);

    if((hdr->classCode != PCI_IDE_CLASS_CODE)
        || (hdr->subclass != PCI_IDE_SUBCLASS_CODE))
    {
        LOG(SYSLOG_ERROR, "This device is not a PCI IDE controller\n");
        MmFreeKernelHeap(hdr);
        return SYSTEM_INCOMPATIBLE;
    }

    device->mainDeviceObject->flags = IO_DEVICE_FLAG_ENUMERATION_CAPABLE;
    device->mainDeviceObject->type = IO_DEVICE_TYPE_STORAGE;
    IoSetDeviceDisplayedName(device, "IDE storage controller");

    if(hdr->progIf & PCI_IDE_PROGIF_OPMODE_PRIMARY_NATIVE)
    {
        if(hdr->standard.bar[PCI_IDE_COMMAND_PRIMARY_BAR] & PCI_IDE_BAR_IO_PORT_FLAG)
        {
            if(0 == 
                (info->channel[PCI_IDE_CHANNEL_PRIMARY].cmdPort 
                    = hdr->standard.bar[PCI_IDE_COMMAND_PRIMARY_BAR] & PCI_IDE_BAR_COMMAND_IO_PORT_MASK))
            {
                LOG(SYSLOG_ERROR, "Primary channel is in native mode, but its command I/O port is 0\n");
                MmFreeKernelHeap(hdr);
                return SYSTEM_INCOMPATIBLE;
            }
        } 
        else
        {
            LOG(SYSLOG_ERROR, "Primary channel has no I/O command port. Hardware bug?\n");
            MmFreeKernelHeap(hdr);
            return SYSTEM_INCOMPATIBLE;
        }
        if(hdr->standard.bar[PCI_IDE_CONTROL_PRIMARY_BAR] & PCI_IDE_BAR_IO_PORT_FLAG)
        {
            if(0 == 
                (info->channel[PCI_IDE_CHANNEL_PRIMARY].controlPort 
                    = hdr->standard.bar[PCI_IDE_CONTROL_PRIMARY_BAR] & PCI_IDE_BAR_CONTROL_IO_PORT_MASK))
            {
                LOG(SYSLOG_ERROR, "Primary channel is in native mode, but its control I/O port is 0\n");
                MmFreeKernelHeap(hdr);
                return SYSTEM_INCOMPATIBLE;
            }
        } 
        else
        {
            LOG(SYSLOG_ERROR, "Primary channel has no I/O control port. Hardware bug?\n");
            MmFreeKernelHeap(hdr);
            return SYSTEM_INCOMPATIBLE;
        }
    }
    else
    {
        info->channel[PCI_IDE_CHANNEL_PRIMARY].cmdPort = PCI_IDE_DEFAULT_PRIMARY_COMMAND_PORT;
        info->channel[PCI_IDE_CHANNEL_PRIMARY].controlPort = PCI_IDE_DEFAULT_PRIMARY_CONTROL_PORT;
    }
    
    if(hdr->progIf & PCI_IDE_PROGIF_OPMODE_SECONDARY_NATIVE)
    {
        if(hdr->standard.bar[PCI_IDE_COMMAND_SECONDARY_BAR] & PCI_IDE_BAR_IO_PORT_FLAG)
        {
            if(0 == 
                (info->channel[PCI_IDE_CHANNEL_SECONDARY].cmdPort 
                    = hdr->standard.bar[PCI_IDE_COMMAND_SECONDARY_BAR] & PCI_IDE_BAR_COMMAND_IO_PORT_MASK))
            {
                LOG(SYSLOG_ERROR, "Secondary channel is in native mode, but its command I/O port is 0\n");
                MmFreeKernelHeap(hdr);
                return SYSTEM_INCOMPATIBLE;
            }
        } 
        else
        {
            LOG(SYSLOG_ERROR, "Secondary channel has no I/O command port. Hardware bug?\n");
            MmFreeKernelHeap(hdr);
            return SYSTEM_INCOMPATIBLE;
        }
        if(hdr->standard.bar[PCI_IDE_CONTROL_SECONDARY_BAR] & PCI_IDE_BAR_IO_PORT_FLAG)
        {
            if(0 == 
                (info->channel[PCI_IDE_CHANNEL_SECONDARY].controlPort 
                    = hdr->standard.bar[PCI_IDE_CONTROL_SECONDARY_BAR] & PCI_IDE_BAR_CONTROL_IO_PORT_MASK))
            {
                LOG(SYSLOG_ERROR, "Secondary channel is in native mode, but its control I/O port is 0\n");
                MmFreeKernelHeap(hdr);
                return SYSTEM_INCOMPATIBLE;
            }
        } 
        else
        {
            LOG(SYSLOG_ERROR, "Secondary channel has no I/O control port. Hardware bug?\n");
            MmFreeKernelHeap(hdr);
            return SYSTEM_INCOMPATIBLE;
        }
        
    }
    else
    {
        info->channel[PCI_IDE_CHANNEL_SECONDARY].cmdPort = PCI_IDE_DEFAULT_SECONDARY_COMMAND_PORT;
        info->channel[PCI_IDE_CHANNEL_SECONDARY].controlPort = PCI_IDE_DEFAULT_SECONDARY_CONTROL_PORT;
    }

    if(hdr->progIf & PCI_IDE_PROGIF_BUS_MASTER_CAPABLE)
    {
        if(hdr->standard.bar[PCI_IDE_BUS_MASTER_BAR] & PCI_IDE_BAR_IO_PORT_FLAG)
        {
            uint16_t port = hdr->standard.bar[PCI_IDE_BUS_MASTER_BAR] & PCI_IDE_BAR_BUS_MASTER_IO_PORT_MASK;
            if(0 == port)
            {
                LOG(SYSLOG_WARNING, "Bus master I/O port is 0\n");
            }
            else
            {
                LOG(SYSLOG_INFO, "Controller is bus master capable\n");
                info->channel[PCI_IDE_CHANNEL_PRIMARY].masterPort = port;
                info->channel[PCI_IDE_CHANNEL_SECONDARY].masterPort = port + 8;
                info->busMaster = true;
                hdr->command |= PCI_HEADER_COMMAND_BUS_MASTER;
            }
        }
    }

    hdr->command |= PCI_HEADER_COMMAND_IO_SPACE;

    rp->code = IO_RP_SET_DEVICE_CONFIGURATION;
    rp->payload.deviceConfiguration.type = IO_BUS_TYPE_PCI;
    rp->payload.deviceConfiguration.device = device;
    rp->payload.deviceConfiguration.offset = offsetof(struct IoPciDeviceHeader, command);
    rp->size = 1;
    rp->buffer = &(hdr->command);
    rp->flags = IO_DRIVER_RP_FLAG_SYNCHRONOUS;

    if(OK != (status = IoSendRp(device->attachedTo, NULL, rp)))
    {
        LOG(SYSLOG_ERROR, "Setting PCI configuration failed with status 0x%X\n", status);
        MmFreeKernelHeap(hdr);
        MmFreeKernelHeap(rp);
        return status;
    }

    if(info->busMaster)
    {
        if(OK != (status = IdeInitializePrdTables(info)))
        {
            MmFreeKernelHeap(hdr);
            MmFreeKernelHeap(rp);
            return status;
        }
    }

    info->initialized = true;
    LOG(SYSLOG_INFO, "Controller succesfully initialized\n");
    
    MmFreeKernelHeap(hdr);
    return status;
}