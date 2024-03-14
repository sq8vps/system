#include "part.h"
#include "mm/heap.h"
#include "mm/mm.h"
#include "ke/core/mutex.h"
#include <stdatomic.h>

#define DISK_DEVICE_FILE_NAME_PREFIX "hd"

static STATUS DiskCreateDeviceFile(struct IoDeviceObject *dev, struct DiskData *info)
{
    char name[5] = DISK_DEVICE_FILE_NAME_PREFIX;

    if(!info->isPartition0)
    {
        uint32_t parentIndex = ((struct DiskData*)(info->part0device->privateData))->index;
        if(parentIndex > 25)
            return OUT_OF_RESOURCES;
        

    }

    
            
}

static STATUS DiskReadSync(struct IoDeviceObject *bdo, uint64_t offset, uint64_t size, void **buffer)
{
    STATUS status = OK;
    struct MmMemoryDescriptor *list = NULL;
    struct IoRp *rp = NULL;
    bool useDirectIo = !!(bdo->flags & IO_DEVICE_FLAG_DIRECT_IO);

    size = ALIGN_UP(size, bdo->blockSize);

    //allocate buffer for reading
    *buffer = MmAllocateKernelHeapAligned(size, (bdo->alignment > bdo->blockSize) ? bdo->alignment : bdo->blockSize);
    if(NULL == *buffer)
        return OUT_OF_RESOURCES;

    if(useDirectIo)
    {
        //build Memory Descriptor List for buffer
        list = MmBuildMemoryDescriptorList(*buffer, size);
        if(NULL == list)
        {
            status = OUT_OF_RESOURCES;
            goto _DiskReadSyncExit;
        }
    }

    //send RP to read data
    rp = IoCreateRp();
    if(NULL == rp)
    {
        status = OUT_OF_RESOURCES;
        goto _DiskReadSyncExit;
    }
    rp->code = IO_RP_READ;
    if(useDirectIo)
    {
        rp->payload.read.memory = list;
        rp->payload.read.systemBuffer = NULL;
    }
    else
    {
        rp->payload.read.memory = NULL;
        rp->payload.read.systemBuffer = *buffer;   
    }
    rp->payload.read.offset = offset;
    rp->size = size;
    status = IoSendRp(bdo, rp);
    if(OK == status)
    {
        IoWaitForRpCompletion(rp);
        status = rp->status;
    }

    if(OK != status)
        MmFreeKernelHeap(buffer);

_DiskReadSyncExit:
    IoFreeRp(rp);
    MmFreeMemoryDescriptorList(list);
    return status;
}

static STATUS DiskAnalyzeMbr(struct IoDeviceObject *bdo, struct DiskData *info)
{
    STATUS status = OK;
    void *buffer = NULL;

    if(NULL != info->mbr)
        MmFreeKernelHeap(info->mbr);
    info->mbr = NULL;

    uint64_t size = (bdo->blockSize > MBR_SIZE_ON_DISK) ? bdo->blockSize : MBR_SIZE_ON_DISK;
    status = DiskReadSync(bdo, MBR_LBA_OFFSET * bdo->blockSize, size, &buffer);
    if(OK != status)
    {
        return status;
    }

    //allocate space for MBR structure
    info->mbr = MmAllocateKernelHeap(sizeof(*(info->mbr)));
    if(NULL == info->mbr)
    {
        MmFreeKernelHeap(buffer);
        return OUT_OF_RESOURCES;
    }
    info->isMbr = !!DiskMbrParse(buffer, info->mbr);
    if(!info->isMbr)
    {
        MmFreeKernelHeap(info->mbr);
        info->mbr = NULL;
    }
    MmFreeKernelHeap(buffer);
    return status;
}

STATUS DiskInitializeVolume(struct IoDeviceObject *bdo, struct IoDeviceObject *dev, struct DiskData *info)
{
    STATUS status;
    status = DiskAnalyzeMbr(bdo, info);
    if(OK != status)
        return status;
    
    if(info->isGpt)
    {
    
    }
    else if(info->isMbr)
    {
        for(uint32_t i = 0; i < sizeof(info->mbr->partition) / sizeof(info->mbr->partition[0]); i++)
        {
            if(DiskMbrIsPartitionUsable(&(info->mbr->partition[i])))
            {
                struct IoDeviceObject *partitionDev;
                status = IoCreateDevice(dev->driverObject, IO_DEVICE_TYPE_DISK, 0, &partitionDev);
                if(OK != status)
                    return status;
                
                if(dev->flags & IO_DEVICE_FLAG_DIRECT_IO)
                    partitionDev->flags |= IO_DEVICE_FLAG_DIRECT_IO;
                if(dev->flags & IO_DEVICE_FLAG_BUFFERED_IO)
                    partitionDev->flags |= IO_DEVICE_FLAG_BUFFERED_IO;
                
                partitionDev->privateData = MmAllocateKernelHeapZeroed(sizeof(struct DiskData));
                if(NULL == partitionDev->privateData)
                {
                    IoDestroyDevice(partitionDev);
                    return OUT_OF_RESOURCES;
                }
                
                struct DiskData *partitionInfo = partitionDev->privateData;
                partitionInfo->isMdo = 0;
                partitionInfo->isPartition0 = 0;
                partitionInfo->part0device = dev;
                partitionInfo->partition.start = info->mbr->partition[i].lba;
                partitionInfo->partition.size = info->mbr->partition[i].sectors;
                partitionInfo->partition.sectorSize = dev->blockSize;
                partitionInfo->index = atomic_fetch_add(&(info->childCount), 1);

                partitionInfo->partition.startBytes = info->mbr->partition[i].lba * dev->blockSize;
                partitionInfo->partition.sizeBytes = info->mbr->partition[i].sectors * dev->blockSize;

                status = IoRegisterDevice(partitionDev, dev);
                if(OK != status)
                {
                    IoDestroyDevice(partitionDev);
                    return status;
                }
            }
        }
    }
    return OK;
}