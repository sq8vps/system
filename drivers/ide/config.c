#include "config.h"
#include "logging.h"
#include "mm/heap.h"
#include "mm/palloc.h"
#include "mm/dynmap.h"

#define IDE_BUFFER_BLOCK_SIZE 65536

#define IDE_MAX_PRD_ENTRIES 1024
#define IDE_PRD_TABLE_ALIGNMENT 65536

#define IDE_PRD_ENTRY_EOT_FLAG 0x8000


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

#define IDE_BMR_COMMAND_SHIFT 0x00
#define IDE_BMR_STATUS_SHIFT 0x02
#define IDE_BMR_PRDT_SHIFT 0x04

#define IDE_DEFAULT_ISA_IRQ 14


STATUS IdeClearPrdTable(struct IdePrdTable *t)
{
    if((NULL == t) || (NULL == t->table))
        return NULL_POINTER_GIVEN;
    
    CmMemset(t->table, 0, IDE_MAX_PRD_ENTRIES * sizeof(*(t->table)));
    t->entries = 0;
    return OK;
}

STATUS IdeInitializePrdTables(struct IdeControllerData *info)
{
    if(NULL == info)
        return NULL_POINTER_GIVEN;

    for(uint16_t t = 0; t < 2; t++)
    {
        uintptr_t size;
        if((IDE_MAX_PRD_ENTRIES * sizeof(struct IdePrdEntry))
            > (size = MmAllocateContiguousPhysicalMemory(
                IDE_MAX_PRD_ENTRIES * sizeof(struct IdePrdEntry), 
                &(info->channel[t].prdt.physical), 
                IDE_PRD_TABLE_ALIGNMENT)))
        {
            if(0 != info->channel[t].prdt.physical)
                MmFreePhysicalMemory(info->channel[t].prdt.physical, size);
            LOG(SYSLOG_ERROR, "Failed to allocate memory for PRD table\n");
            return OUT_OF_RESOURCES;
        }
        if(NULL == (info->channel[t].prdt.table = MmMapDynamicMemory(info->channel[t].prdt.physical, size, 0)))
        {
            MmFreePhysicalMemory(info->channel[t].prdt.physical, size);
            LOG(SYSLOG_ERROR, "Failed to map memory for PRD table\n");
            return OUT_OF_RESOURCES;
        }
        IdeClearPrdTable(&(info->channel[t].prdt));
    }

    return OK;
}

uint32_t IdeAddPrdEntry(struct IdePrdTable *table, uint32_t address, uint16_t size)
{
    if(table->entries < IDE_MAX_PRD_ENTRIES)
    {
        table->table[table->entries].address = address;
        table->table[table->entries].size = size;
        table->table[table->entries].eot = IDE_PRD_ENTRY_EOT_FLAG;
        if(table->entries > 0)
            table->table[table->entries - 1].eot = 0;
        table->entries++;
    }
    return IDE_MAX_PRD_ENTRIES - table->entries;
}


STATUS IdeConfigureController(struct IoDeviceObject *bdo, struct IoDeviceObject *mdo, struct IdeControllerData *info)
{
    STATUS status = OK;
    
    struct IoPciDeviceHeader *hdr = NULL;
    status = IoReadConfigSpace(bdo, 0, sizeof(struct IoPciDeviceHeader), (void**)&hdr);

    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Getting PCI configuration failed with status 0x%X", status);
        return status;
    }

    if((hdr->classCode != PCI_IDE_CLASS_CODE)
        || (hdr->subclass != PCI_IDE_SUBCLASS_CODE))
    {
        LOG(SYSLOG_ERROR, "This device is not a PCI IDE controller");
        MmFreeKernelHeap(hdr);
        return SYSTEM_INCOMPATIBLE;
    }

    mdo->flags = IO_DEVICE_FLAG_ENUMERATION_CAPABLE;
    mdo->type = IO_DEVICE_TYPE_STORAGE;

    if(hdr->progIf & PCI_IDE_PROGIF_OPMODE_PRIMARY_NATIVE)
    {
        if(hdr->standard.bar[PCI_IDE_COMMAND_PRIMARY_BAR] & PCI_IDE_BAR_IO_PORT_FLAG)
        {
            if(0 == 
                (info->channel[PCI_IDE_CHANNEL_PRIMARY].cmdPort 
                    = hdr->standard.bar[PCI_IDE_COMMAND_PRIMARY_BAR] & PCI_IDE_BAR_COMMAND_IO_PORT_MASK))
            {
                LOG(SYSLOG_ERROR, "Primary channel is in native mode, but its command I/O port is 0");
                MmFreeKernelHeap(hdr);
                return SYSTEM_INCOMPATIBLE;
            }
        } 
        else
        {
            LOG(SYSLOG_ERROR, "Primary channel has no I/O command port. Hardware bug?");
            MmFreeKernelHeap(hdr);
            return SYSTEM_INCOMPATIBLE;
        }
        if(hdr->standard.bar[PCI_IDE_CONTROL_PRIMARY_BAR] & PCI_IDE_BAR_IO_PORT_FLAG)
        {
            if(0 == 
                (info->channel[PCI_IDE_CHANNEL_PRIMARY].controlPort 
                    = hdr->standard.bar[PCI_IDE_CONTROL_PRIMARY_BAR] & PCI_IDE_BAR_CONTROL_IO_PORT_MASK))
            {
                LOG(SYSLOG_ERROR, "Primary channel is in native mode, but its control I/O port is 0");
                MmFreeKernelHeap(hdr);
                return SYSTEM_INCOMPATIBLE;
            }
        } 
        else
        {
            LOG(SYSLOG_ERROR, "Primary channel has no I/O control port. Hardware bug?");
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
                LOG(SYSLOG_ERROR, "Secondary channel is in native mode, but its command I/O port is 0");
                MmFreeKernelHeap(hdr);
                return SYSTEM_INCOMPATIBLE;
            }
        } 
        else
        {
            LOG(SYSLOG_ERROR, "Secondary channel has no I/O command port. Hardware bug?");
            MmFreeKernelHeap(hdr);
            return SYSTEM_INCOMPATIBLE;
        }
        if(hdr->standard.bar[PCI_IDE_CONTROL_SECONDARY_BAR] & PCI_IDE_BAR_IO_PORT_FLAG)
        {
            if(0 == 
                (info->channel[PCI_IDE_CHANNEL_SECONDARY].controlPort 
                    = hdr->standard.bar[PCI_IDE_CONTROL_SECONDARY_BAR] & PCI_IDE_BAR_CONTROL_IO_PORT_MASK))
            {
                LOG(SYSLOG_ERROR, "Secondary channel is in native mode, but its control I/O port is 0");
                MmFreeKernelHeap(hdr);
                return SYSTEM_INCOMPATIBLE;
            }
        } 
        else
        {
            LOG(SYSLOG_ERROR, "Secondary channel has no I/O control port. Hardware bug?");
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
                LOG(SYSLOG_WARNING, "Bus master I/O port is 0");
            }
            else
            {
                LOG(SYSLOG_INFO, "Controller is bus master capable");
                info->channel[PCI_IDE_CHANNEL_PRIMARY].masterPort = port;
                info->channel[PCI_IDE_CHANNEL_SECONDARY].masterPort = port + 8;
                info->busMaster = true;
                hdr->command |= PCI_HEADER_COMMAND_BUS_MASTER;
            }
        }
    }

    hdr->command |= PCI_HEADER_COMMAND_IO_SPACE;

    status = IoWriteConfigSpace(bdo, offsetof(struct IoPciDeviceHeader, command), 1, &(hdr->command));

    if(OK != (status))
    {
        LOG(SYSLOG_ERROR, "Setting PCI configuration failed with status 0x%X", status);
        MmFreeKernelHeap(hdr);
        return status;
    }

    if(info->busMaster)
    {
        if(OK != (status = IdeInitializePrdTables(info)))
        {
            MmFreeKernelHeap(hdr);
            return status;
        }
    }

    struct IoDeviceResource *resource;
    uint32_t resourceCount;
    status = IoGetDeviceResources(bdo, &resource, &resourceCount);
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Getting bus configuration failed with status 0x%X", status);
        MmFreeKernelHeap(hdr);
        return status;
    }

    struct IoIrqEntry *irq = NULL;

    for(uint32_t i = 0; i < resourceCount; i++)
    {
        if(IO_RESOURCE_IRQ == resource[i].type)
        {
            irq = &(resource[i].irq);
            break;
        }
    }

    uint32_t irqInput = 0;
    if((NULL != irq) && (0 != irq->pin))
    {
        irqInput = irq->gsi;
        status = HalRegisterIrq(irqInput, IdeIsr, info, irq->params);
    }
    else if(0xFF != hdr->standard.interruptLine)
    {
        struct HalInterruptParams params;
        params.trigger = IT_TRIGGER_EDGE;
        params.mode = IT_MODE_FIXED;
        params.polarity = IT_POLARITY_ACTIVE_HIGH;
        params.shared = IT_SHAREABLE;
        params.wake = IT_WAKE_INCAPABLE;
        if(0 != hdr->standard.interruptLine)
        {
            irqInput = I686ResolveIsaIrqMapping(hdr->standard.interruptLine);
            status = HalRegisterIrq(irqInput, IdeIsr, info, params);
        }
        else
        {
            irqInput = I686ResolveIsaIrqMapping(IDE_DEFAULT_ISA_IRQ);
            status = HalRegisterIrq(irqInput, IdeIsr, info, params);
        }
    }
    
    if(OK == status)
        status = HalEnableIrq(irqInput, IdeIsr);

    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Setting up IRQ failed with status 0x%X", status);
        MmFreeKernelHeap(hdr);
        return status;        
    }

    if(OK != (status = IoCreateRpQueue(IdeProcessRequest, &(info->channel[PCI_IDE_CHANNEL_PRIMARY].queue))))
    {
        LOG(SYSLOG_ERROR, "RP queue creation failed with status 0x%X", status);
        HalDisableIrq(irqInput, IdeIsr);
        HalUnregisterIrq(irqInput, IdeIsr);
        MmFreeKernelHeap(hdr);
        return status;           
    }

    if(OK != (status = IoCreateRpQueue(IdeProcessRequest, &(info->channel[PCI_IDE_CHANNEL_SECONDARY].queue))))
    {
        LOG(SYSLOG_ERROR, "RP queue creation failed with status 0x%X", status);
        HalDisableIrq(irqInput, IdeIsr);
        HalUnregisterIrq(irqInput, IdeIsr);
        MmFreeKernelHeap(hdr);
        return status;           
    }

    MmFreeKernelHeap(hdr);

    info->initialized = true;
    LOG(SYSLOG_INFO, "Controller succesfully initialized");

    return status;
}

void IdeWriteBmrCommand(struct IdeControllerData *info, uint8_t chan, uint8_t command)
{
    IoPortWriteByte(info->channel[chan].masterPort + IDE_BMR_COMMAND_SHIFT, command);
}

uint8_t IdeReadBmrStatus(struct IdeControllerData *info, uint8_t chan)
{
    return IoPortReadByte(info->channel[chan].masterPort + IDE_BMR_STATUS_SHIFT);
}

void IdeWriteBmrStatus(struct IdeControllerData *info, uint8_t chan, uint8_t status)
{
    IoPortWriteByte(info->channel[chan].masterPort + IDE_BMR_STATUS_SHIFT, status);
}

void IdeWriteBmrPrdt(struct IdeControllerData *info, uint8_t chan, struct IdePrdTable *prdt)
{
    IoPortWriteDWord(info->channel[chan].masterPort + IDE_BMR_PRDT_SHIFT, prdt->physical);
}