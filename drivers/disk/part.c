#include "part.h"
#include "mm/heap.h"
#include "mm/mm.h"

STATUS DiskInitializeVolume(struct IoDeviceObject *bdo, struct DiskData *info)
{
    STATUS status = OK;
    void *buffer = NULL;
    struct MmMemoryDescriptor *list = NULL;
    struct IoRp *rp = NULL;

    if(NULL != info->mbr)
        MmFreeKernelHeap(info->mbr);
    info->mbr = NULL;

    //allocate buffer for reading
    uint64_t size = (bdo->blockSize > MBR_SIZE_ON_DISK) ? bdo->blockSize : MBR_SIZE_ON_DISK;
    buffer = MmAllocateKernelHeapAligned(size, (bdo->alignment > bdo->blockSize) ? bdo->alignment : bdo->blockSize);
    if(NULL == buffer)
        return OUT_OF_RESOURCES;

    //build Memory Descriptor List for buffer
    list = MmBuildMemoryDescriptorList(buffer, size);
    if(NULL == list)
    {
        status = OUT_OF_RESOURCES;
        goto _DiskInitializeVolumeExit;
    }

    //send RP to read data
    rp = IoCreateRp();
    if(NULL == rp)
    {
        status = OUT_OF_RESOURCES;
        goto _DiskInitializeVolumeExit;
    }
    rp->code = IO_RP_READ;
    rp->flags |= IO_DRIVER_RP_FLAG_SYNCHRONOUS;
    rp->payload.read.memory = list;
    rp->payload.read.offset = MBR_LBA_OFFSET * bdo->blockSize;
    rp->size = size;
    status = IoSendRp(bdo, rp);
    if(OK == status)
    {
        if(OK == rp->status)
        {
            //allocate space for MBR structure
            info->mbr = MmAllocateKernelHeap(sizeof(*(info->mbr)));
            if(NULL == info->mbr)
            {
                status = OUT_OF_RESOURCES;
                goto _DiskInitializeVolumeExit;
            }
            status = DiskMbrParse(buffer, info->mbr);
            if(OK != status)
            {
                MmFreeKernelHeap(info->mbr);
                info->mbr = NULL;
            }
        }
        else
            status = rp->status;
    }

_DiskInitializeVolumeExit:
    IoFreeRp(rp);
    MmFreeMemoryDescriptorList(list);
    MmFreeKernelHeap(buffer);
    return status;
}