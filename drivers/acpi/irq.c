#include "device.h"
#include "logging.h"
#include "acpi.h"

static ACPI_STATUS AcpiExtractIrqResource(ACPI_RESOURCE *res, struct IoIrqEntry *irq, uint32_t index)
{
    if((NULL == res) || (NULL == irq))
        return AE_NULL_OBJECT;
    
    if(ACPI_RESOURCE_TYPE_IRQ == res->Type)
    {
        if(res->Length == 2)
        {
            irq->params.polarity = IT_POLARITY_ACTIVE_HIGH;
            irq->params.trigger = IT_TRIGGER_EDGE;
            irq->params.wake = IT_WAKE_INCAPABLE;
            irq->params.shared = IT_NOT_SHAREABLE;
        }
        else
        {
            irq->params.polarity = res->Data.Irq.Polarity ? IT_POLARITY_ACTIVE_LOW : IT_POLARITY_ACTIVE_HIGH;
            irq->params.trigger = res->Data.Irq.Triggering ? IT_TRIGGER_EDGE : IT_TRIGGER_LEVEL;
            irq->params.wake = res->Data.Irq.WakeCapable ? IT_WAKE_CAPABLE : IT_WAKE_INCAPABLE;
            irq->params.shared = res->Data.Irq.Shareable ? IT_SHAREABLE : IT_NOT_SHAREABLE;
        }
        //ACPICA spec says absolutely nothing about how to handle these structures
        //ACPI says it should be a 16-bit PIC IRQ bit map, but this structure has no 16-bit field
        //below is how its done in Minix
        irq->gsi = res->Data.Irq.Interrupts[index];
    }
    else if(ACPI_RESOURCE_TYPE_EXTENDED_IRQ == res->Type)
    {
        irq->params.polarity = res->Data.ExtendedIrq.Polarity ? IT_POLARITY_ACTIVE_LOW : IT_POLARITY_ACTIVE_HIGH;
        irq->params.trigger = res->Data.ExtendedIrq.Triggering ? IT_TRIGGER_EDGE : IT_TRIGGER_LEVEL;
        irq->params.wake = res->Data.ExtendedIrq.WakeCapable ? IT_WAKE_CAPABLE : IT_WAKE_INCAPABLE;
        irq->params.shared = res->Data.ExtendedIrq.Shareable ? IT_SHAREABLE : IT_NOT_SHAREABLE;
        irq->gsi = res->Data.ExtendedIrq.Interrupts[index];
    }
    else
        return AE_TYPE;
    return AE_OK;
}

ACPI_STATUS AcpiGetPciIrqTree(ACPI_HANDLE device, struct IoIrqMap *map)
{
    if((NULL == device) || (NULL == map))
        return AE_NULL_OBJECT;
    
    ACPI_STATUS status;
    ACPI_BUFFER prtBuf;
    prtBuf.Pointer = NULL;
    prtBuf.Length = ACPI_ALLOCATE_BUFFER;
    //try to get PCI IRQ Routing Table
    //if this operation fails (for whatever reason), assume the device is not a PCI bridge
    //so there is no point in checking deeper
    status = AcpiGetIrqRoutingTable(device, &prtBuf);
    if(ACPI_FAILURE(status))
    {
        if(NULL != prtBuf.Pointer)
            AcpiOsFree(prtBuf.Pointer);
        map->irqCount = 0;
        map->child = NULL;
        return AE_OK;
    }

    //get device info to obtain device address
    ACPI_DEVICE_INFO *info;
    status = AcpiGetObjectInfo(device, &info);
    if(ACPI_FAILURE(status))
    {
        //treat failure as the end of the tree
        if(NULL != info)
            ACPI_FREE(info);
        map->irqCount = 0;
        map->child = NULL;
        return AE_OK;
    }

    if(0 == (info->Valid & ACPI_VALID_ADR))
    {
        //treat missing address field as the end of the tree
        ACPI_FREE(info);
        map->irqCount = 0;
        map->child = NULL;
        return AE_OK;
    }

    //store PCI device location
    map->id.pci.bus = 0; //bus number is unknown at this point and is determined by the upstream bridge
    map->id.pci.device = PCI_ADR_EXTRACT_DEVICE(info->Address);
    map->id.pci.function = PCI_ADR_EXTRACT_FUNCTION(info->Address);

    ACPI_FREE(info);

    //calculate number of interrupt entries first
    //just calculate number of PRT entries
    ACPI_PCI_ROUTING_TABLE *prt = prtBuf.Pointer;
    uint32_t irqCount = 0;
    while(prt->Length > 0)
    {
        irqCount++;
        prt = (ACPI_PCI_ROUTING_TABLE*)((uintptr_t)prt + prt->Length);
    }

    map->irq = AcpiOsAllocateZeroed(sizeof(*(map->irq)) * irqCount);
    if(NULL == map->irq)
    {
        AcpiOsFree(prtBuf.Pointer);
        map->irqCount = 0;
        map->child = NULL;
        return AE_NO_MEMORY;
    }

    prt = prtBuf.Pointer;
    uint32_t i = 0;
    //get PCI Routing Table entries
    //parse and store them
    while(prt->Length > 0) //loop for all elements
    {
        if(prt->Source[0] == '\0') //standard entry, parameters are given explicitely
        {
            map->irq[i].gsi = prt->SourceIndex;
            map->irq[i].pin = prt->Pin + 1;
            map->irq[i].params.polarity = IT_POLARITY_ACTIVE_LOW;
            map->irq[i].params.trigger = IT_TRIGGER_LEVEL;
            map->irq[i].params.wake = IT_WAKE_INCAPABLE;
            map->irq[i].params.shared = IT_SHAREABLE;
            map->irq[i].id.pci.bus = 0;
            map->irq[i].id.pci.device = PCI_ADR_EXTRACT_DEVICE(prt->Address);
            map->irq[i].id.pci.function = PCI_ADR_EXTRACT_FUNCTION(prt->Address);
            i++;
        }
        else //irq routing via PCI Interrupt Link Device
        {
            ACPI_HANDLE linkDev;
            if(ACPI_SUCCESS(AcpiGetHandle(device, prt->Source, &linkDev))) //get link device
            {
                ACPI_BUFFER resBuf;
                resBuf.Length = ACPI_ALLOCATE_BUFFER;
                resBuf.Pointer = NULL;
                if(ACPI_SUCCESS(AcpiGetCurrentResources(linkDev, &resBuf))) //get current link device resources
                {
                    ACPI_RESOURCE *res = resBuf.Pointer;
                    while(ACPI_RESOURCE_TYPE_END_TAG != res->Type)
                    {
                        if((ACPI_RESOURCE_TYPE_IRQ == res->Type)
                            || (ACPI_RESOURCE_TYPE_EXTENDED_IRQ == res->Type))
                        {
                            AcpiExtractIrqResource(res, &(map->irq[i]), prt->SourceIndex);
                            map->irq[i].pin = prt->Pin + 1;
                            map->irq[i].id.pci.bus = 0;
                            map->irq[i].id.pci.device = PCI_ADR_EXTRACT_DEVICE(prt->Address);
                            map->irq[i].id.pci.function = PCI_ADR_EXTRACT_FUNCTION(prt->Address);
                            i++;
                        }
                        res = (ACPI_RESOURCE*)(((char*)res) + res->Length);
                    }
                }
                if(NULL != resBuf.Pointer)
                    AcpiOsFree(resBuf.Pointer);
            }
        }
        prt = (ACPI_PCI_ROUTING_TABLE*)((uintptr_t)prt + prt->Length);
    }
    AcpiOsFree(prtBuf.Pointer);

    map->irqCount = i;

    //get all children devices
    ACPI_HANDLE child = NULL;
    while(ACPI_SUCCESS(AcpiGetNextObject(ACPI_TYPE_DEVICE, device, child, &child)))
    {
        //build map recusrively
        if(NULL == map->child)
        {
            map->child = MmAllocateKernelHeap(sizeof(*(map->child)));
            if(NULL == map->child)
                return AE_NO_MEMORY;
            AcpiGetPciIrqTree(child, map->child);
            if((0 == map->child->irqCount) && (NULL == map->child->child))
            {
                MmFreeKernelHeap(map->child);
                map->child = NULL;
            }
        }
        else
        {
            struct IoIrqMap *childMap = map->child;
            while(NULL != childMap->next)
                childMap = childMap->next;
            childMap->next = MmAllocateKernelHeap(sizeof(*(childMap->next)));
            if(NULL == childMap->next)
                return AE_NO_MEMORY;
            AcpiGetPciIrqTree(child, childMap->next);
            if((0 == childMap->next->irqCount) && (NULL == childMap->next->child))
            {
                MmFreeKernelHeap(childMap->next);
                childMap->next = NULL;
            }
        }
    }
    return AE_OK;
}

ACPI_STATUS AcpiGetIrq(ACPI_HANDLE device, struct IoDeviceResource *devRes)
{
    if((NULL == device) || (NULL == devRes))
        return AE_NULL_OBJECT;
    ACPI_STATUS status;
    ACPI_BUFFER buf;
    buf.Pointer = NULL;
    buf.Length = ACPI_ALLOCATE_BUFFER;
    status = AcpiGetCurrentResources(device, &buf);
    if(ACPI_FAILURE(status))
        return status;

    ACPI_RESOURCE *res = buf.Pointer;

    uint32_t i = 0;
    while(ACPI_RESOURCE_TYPE_END_TAG != res->Type)
    {
        if((ACPI_RESOURCE_TYPE_IRQ == res->Type)
            || (ACPI_RESOURCE_TYPE_EXTENDED_IRQ == res->Type))
        {
            AcpiExtractIrqResource(res, &(devRes[i].irq), 0);
            devRes[i].type = IO_RESOURCE_IRQ;
            i++;
        }
        res = (ACPI_RESOURCE*)(((char*)res) + res->Length);
    }
    AcpiOsFree(buf.Pointer);
    
    return AE_OK;
}

uint32_t AcpiGetResourceCount(ACPI_HANDLE device)
{
    uint32_t count = 0;
    if(NULL == device)
        return 0;
    ACPI_STATUS status;
    ACPI_BUFFER buf;
    buf.Pointer = NULL;
    buf.Length = ACPI_ALLOCATE_BUFFER;
    status = AcpiGetCurrentResources(device, &buf);
    if(ACPI_FAILURE(status))
        return 0;

    ACPI_RESOURCE *res = buf.Pointer;

    //count resources
    while(ACPI_RESOURCE_TYPE_END_TAG != res->Type)
    {
        switch(res->Type)
        {
            case ACPI_RESOURCE_TYPE_IRQ:
            case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
                count++;
                break;
            default:
                break;
        }
        res = (ACPI_RESOURCE*)(((char*)res) + res->Length);
    }
    AcpiOsFree(buf.Pointer);
    return count; 
}

ACPI_STATUS AcpiFillResourceList(ACPI_HANDLE device, struct AcpiDeviceInfo *info)
{
    if(0 != info->resourceCount)
        MmFreeKernelHeap(info->resource);
    
    info->resourceCount = 0;
    
    //calculate number of resource entries
    //PCI host bridge provides exactly one IRQ map
    if(ACPI_IS_PCI_HOST_BRIDGE(info->pnpId))
    {
        info->resourceCount++;
    }

    info->resourceCount += AcpiGetResourceCount(device);

    if(0 != info->resourceCount)
    {
        info->resource = MmAllocateKernelHeapZeroed(info->resourceCount * sizeof(*(info->resource)));
        if(NULL == info->resource)
        {
            info->resourceCount = 0;
            return AE_NO_MEMORY;
        }

        //get resources
        uint32_t i = 0;
        //if this is a PCI host bridge, invoke PRT method to get IRQ map
        if(ACPI_IS_PCI_HOST_BRIDGE(info->pnpId))
        {
            if(ACPI_SUCCESS(AcpiGetPciIrqTree(device, &(info->resource[i].irqMap))))
                info->resource[i].type = IO_RESOURCE_IRQ_MAP;
            i++;
        }

        return AcpiGetIrq(device, &(info->resource[i]));
    }
    else
        return OK;
}