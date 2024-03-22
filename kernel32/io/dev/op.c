#include "op.h"
#include "io/dev/dev.h"
#include "io/dev/rp.h"
#include "mm/mm.h"
#include "mm/heap.h"
#include "common.h"

STATUS IoReadDeviceSync(struct IoDeviceObject *dev, uint64_t offset, uint64_t size, void **buffer)
{
    STATUS status = OK;
    struct MmMemoryDescriptor *list = NULL;
    struct IoRp *rp = NULL;
    bool useDirectIo = !!(dev->flags & IO_DEVICE_FLAG_DIRECT_IO);
    uintptr_t alignment = (dev->alignment > dev->blockSize) ? dev->alignment : dev->blockSize;
    uint64_t alignedSize = 0;
    uint64_t alignedOffset = 0;
    
    if(0 == (dev->flags & (IO_DEVICE_FLAG_DIRECT_IO | IO_DEVICE_FLAG_BUFFERED_IO)))
        return OPERATION_NOT_ALLOWED;

    if(0 == alignment)
        alignment = 1;
    
    alignedOffset = ALIGN_DOWN(offset, alignment);

    if(dev->blockSize >= 1)
        alignedSize = ALIGN_UP(size + (offset - alignedOffset), dev->blockSize);

    uint8_t *alignedBuffer = MmAllocateKernelHeapAligned(alignedSize, alignment);
    if(NULL == alignedBuffer)
        return OUT_OF_RESOURCES;

    if(useDirectIo)
    {
        list = MmBuildMemoryDescriptorList(alignedBuffer, alignedSize);
        if(NULL == list)
        {
            MmFreeKernelHeap(alignedBuffer);
            status = OUT_OF_RESOURCES;
            goto _IoDeviceReadSyncExit;
        }
    }

    rp = IoCreateRp();
    if(NULL == rp)
    {
        MmFreeKernelHeap(alignedBuffer);
        status = OUT_OF_RESOURCES;
        goto _IoDeviceReadSyncExit;
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
        rp->payload.read.systemBuffer = alignedBuffer;   
    }
    rp->payload.read.offset = alignedOffset;
    rp->size = alignedSize;
    status = IoSendRp(dev, rp);
    if(OK == status)
    {
        IoWaitForRpCompletion(rp);
        status = rp->status;
    }

    if(OK != status)
        MmFreeKernelHeap(alignedBuffer);
    else
    {
        //check if requested offset was aligned correctly
        if(alignedOffset == offset)
        {
            *buffer = alignedBuffer;
        }
        else
        {
            //unaligned offset, double buffering required
            *buffer = MmAllocateKernelHeap(size);
            if(NULL == *buffer)
            {
                MmFreeKernelHeap(alignedBuffer);
                status = OUT_OF_RESOURCES;
            }

            CmMemcpy(*buffer, &(alignedBuffer[offset - alignedOffset]), size);
            MmFreeKernelHeap(alignedBuffer);
        }
    }

_IoDeviceReadSyncExit:
    IoFreeRp(rp);
    MmFreeMemoryDescriptorList(list);
    return status;
}