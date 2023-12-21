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
            irq->polarity = IT_POLARITY_ACTIVE_HIGH;
            irq->trigger = IT_TRIGGER_EDGE;
            irq->wake = IT_WAKE_INCAPABLE;
            irq->sharing = IT_NOT_SHARED;
        }
        else
        {
            irq->polarity = res->Data.Irq.Polarity ? IT_POLARITY_ACTIVE_LOW : IT_POLARITY_ACTIVE_HIGH;
            irq->trigger = res->Data.Irq.Triggering ? IT_TRIGGER_EDGE : IT_TRIGGER_LEVEL;
            irq->wake = res->Data.Irq.WakeCapable ? IT_WAKE_CAPABLE : IT_WAKE_INCAPABLE;
            irq->sharing = res->Data.Irq.Shareable ? IT_SHARED : IT_NOT_SHARED;
        }
        //ACPICA spec says absolutely nothing about how to handle these structures
        //ACPI says it should be a 16-bit PIC IRQ bit map, but this structure has no 16-bit field
        //below is how its done in Minix
        irq->gsi = res->Data.Irq.Interrupts[index];
    }
    else if(ACPI_RESOURCE_TYPE_EXTENDED_IRQ == res->Type)
    {
        irq->polarity = res->Data.ExtendedIrq.Polarity ? IT_POLARITY_ACTIVE_LOW : IT_POLARITY_ACTIVE_HIGH;
        irq->trigger = res->Data.ExtendedIrq.Triggering ? IT_TRIGGER_EDGE : IT_TRIGGER_LEVEL;
        irq->wake = res->Data.ExtendedIrq.WakeCapable ? IT_WAKE_CAPABLE : IT_WAKE_INCAPABLE;
        irq->sharing = res->Data.ExtendedIrq.Shareable ? IT_SHARED : IT_NOT_SHARED;
        irq->gsi = res->Data.ExtendedIrq.Interrupts[index];
    }
    else
        return AE_TYPE;
    return AE_OK;
}

ACPI_STATUS AcpiGetPciIrqTree(ACPI_HANDLE device, struct IoIrqMap **map)
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
        *map = NULL;
        return AE_OK;
    }

    *map = AcpiOsAllocateZeroed(sizeof(**map));
    if(NULL == *map)
        return AE_NO_MEMORY;

    //get device info to obtain device address
    ACPI_DEVICE_INFO *info;
    status = AcpiGetObjectInfo(device, &info);
    if(ACPI_FAILURE(status))
    {
        //treat failure as the end of the tree
        if(NULL != info)
            ACPI_FREE(info);
        *map = NULL;
        return AE_OK;
    }

    if(0 == (info->Valid & ACPI_VALID_ADR))
    {
        //treat missing address field as the end of the tree
        ACPI_FREE(info);
        *map = NULL;
        return AE_OK;
    }

    //store PCI device location
    (*map)->id.pci.bus = 0; //bus number is unknown at this point and is determined by the upstream bridge
    (*map)->id.pci.device = PCI_ADR_EXTRACT_DEVICE(info->Address);
    (*map)->id.pci.function = PCI_ADR_EXTRACT_FUNCTION(info->Address);

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

    (*map)->irq = AcpiOsAllocateZeroed(sizeof(*((*map)->irq)) * irqCount);
    if(NULL == (*map)->irq)
    {
        AcpiOsFree(prtBuf.Pointer);
        AcpiOsFree(*map);
        *map = NULL;
        return AE_NO_MEMORY;
    }

    prt = prtBuf.Pointer;
    uint32_t i = 0;
    //get PCI Routing Table entries
    //parse and store them
    while(prt->Length > 0) //loop for all elements
    {
        if(prt->Source[0] == '\0') //standard entry, parameter are given explicitely
        {
            (*map)->irq[i].gsi = prt->SourceIndex;
            (*map)->irq[i].pin = prt->Pin + 1;
            (*map)->irq[i].polarity = IT_POLARITY_ACTIVE_LOW;
            (*map)->irq[i].trigger = IT_TRIGGER_LEVEL;
            (*map)->irq[i].wake = IT_WAKE_INCAPABLE;
            (*map)->irq[i].sharing = IT_SHARED;
            (*map)->irq[i].id.pci.bus = 0;
            (*map)->irq[i].id.pci.device = PCI_ADR_EXTRACT_DEVICE(prt->Address);
            (*map)->irq[i].id.pci.function = PCI_ADR_EXTRACT_FUNCTION(prt->Address);
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
                if(ACPI_SUCCESS(AcpiGetCurrentResources(linkDev, &resBuf))) //get link device resources
                {
                    ACPI_RESOURCE *res = resBuf.Pointer;
                    while(ACPI_RESOURCE_TYPE_END_TAG != res->Type)
                    {
                        if((ACPI_RESOURCE_TYPE_IRQ == res->Type)
                            || (ACPI_RESOURCE_TYPE_EXTENDED_IRQ == res->Type))
                        {
                            AcpiExtractIrqResource(res, &((*map)->irq[i]), prt->SourceIndex);
                            (*map)->irq[i].pin = prt->Pin + 1;
                            (*map)->irq[i].id.pci.bus = 0;
                            (*map)->irq[i].id.pci.device = PCI_ADR_EXTRACT_DEVICE(prt->Address);
                            (*map)->irq[i].id.pci.function = PCI_ADR_EXTRACT_FUNCTION(prt->Address);
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

    (*map)->irqCount = i;

    //get all children devices
    ACPI_HANDLE child = NULL;
    while(ACPI_SUCCESS(AcpiGetNextObject(ACPI_TYPE_DEVICE, device, child, &child)))
    {
        //build map recusrively
        if(NULL == (*map)->child)
            AcpiGetPciIrqTree(child, &((*map)->child));
        else
        {
            struct IoIrqMap *childMap = (*map)->child;
            while(NULL != childMap->next)
                childMap = childMap->next;
            AcpiGetPciIrqTree(child, &(childMap->next));
        }
    }
    return AE_OK;
}

ACPI_STATUS AcpiGetIrq(ACPI_HANDLE device, struct BusSubDeviceInfo *info)
{
    if((NULL == device) || (NULL == info))
        return AE_NULL_OBJECT;
    ACPI_STATUS status;
    ACPI_BUFFER buf;
    buf.Pointer = NULL;
    buf.Length = ACPI_ALLOCATE_BUFFER;
    status = AcpiGetCurrentResources(device, &buf);
    if(ACPI_FAILURE(status))
        return status;
    ACPI_RESOURCE *res = buf.Pointer;
    uint32_t irqCount = 0;
    while(ACPI_RESOURCE_TYPE_END_TAG != res->Type)
    {
        if((ACPI_RESOURCE_TYPE_IRQ == res->Type)
            || (ACPI_RESOURCE_TYPE_EXTENDED_IRQ == res->Type))
        {
            irqCount++;
        }
        res = (ACPI_RESOURCE*)(((char*)res) + res->Length);
    }
    if(0 == irqCount)
    {
        AcpiOsFree(buf.Pointer);
        return AE_OK;
    }
    info->irqMap = AcpiOsAllocateZeroed(sizeof(*(info->irqMap)));
    if(NULL == info->irqMap)
    {
        AcpiOsFree(buf.Pointer);
        return AE_NO_MEMORY;
    }

    info->irqMap->irq = AcpiOsAllocateZeroed(sizeof(*(info->irqMap->irq)) * irqCount);
    if(NULL == info->irqMap->irq)
    {
        AcpiOsFree(buf.Pointer);
        AcpiOsFree(info->irqMap);
        info->irqMap = NULL;
        return AE_NO_MEMORY;
    }
    res = buf.Pointer;
    uint32_t i = 0;
    while(ACPI_RESOURCE_TYPE_END_TAG != res->Type)
    {
        if((ACPI_RESOURCE_TYPE_IRQ == res->Type)
            || (ACPI_RESOURCE_TYPE_EXTENDED_IRQ == res->Type))
        {
            AcpiExtractIrqResource(res, &(info->irqMap->irq[i]), 0);
            i++;
        }
        res = (ACPI_RESOURCE*)(((char*)res) + res->Length);
    }
    AcpiOsFree(buf.Pointer);
    
    info->irqMap->irqCount = i;
    info->irqMap->next = NULL;
    info->irqMap->child = NULL;
    info->irqMap->id = info->id;
    return AE_OK;
}