#include "op.h"
#include "io/dev/dev.h"
#include "io/dev/rp.h"
#include "mm/mm.h"
#include "mm/heap.h"
#include "rtl/string.h"

struct IoReadWriteCallbackContext
{
    struct IoDeviceObject *dev;
    struct IoVfsNode *node;
    struct MmMemoryDescriptor *list; //Physical Memory Descriptor list for caller buffer
    struct MmMemoryDescriptor *alignedList;
    uint64_t alignedOffset; //aligned buffer offset
    uint8_t *alignedBuffer; //aligned buffer
    size_t alignedSize;
    size_t size;
    uint64_t offset;
    bool fullDirect;
    bool useDirectIo;
    bool write;
    bool firstWriteStep;

    IoReadWriteCompletionCallback callback;
    void *context;
};

static STATUS IoReadWriteCallback(struct IoRp *rp, void *context);

static STATUS IoPerformReadWrite(bool write,
                    struct IoDeviceObject *dev, struct IoVfsNode *node,
                    uint64_t offset, size_t size, uint64_t alignedOffset, size_t alignedSize,
                    void *alignedBuffer,
                    struct MmMemoryDescriptor *list, struct MmMemoryDescriptor *alignedList,
                    bool useDirectIo, bool fullDirect, bool firstWriteStep,
                    IoReadWriteCompletionCallback callback, void *context)
{
    STATUS status = OK;
    struct IoRp *rp = NULL;
    rp = IoCreateRp();
    if(NULL == rp)
    {
        status = OUT_OF_RESOURCES;
        goto IoPerformReadWriteFailure;
    }

    if(write)
    {
        if(useDirectIo)
        {
            if(fullDirect)
                rp->payload.write.memory = list;
            else
                rp->payload.write.memory = alignedList;
            rp->payload.write.systemBuffer = NULL;
        }
        else
        {
            rp->payload.write.memory = NULL;
            rp->payload.write.systemBuffer = alignedBuffer;   
        }
    }
    else
    {
        if(useDirectIo)
        {
            if(fullDirect)
                rp->payload.read.memory = list;
            else
                rp->payload.read.memory = alignedList;
            rp->payload.read.systemBuffer = NULL;
        }
        else
        {
            rp->payload.read.memory = NULL;
            rp->payload.read.systemBuffer = alignedBuffer;   
        }
    }
    
    struct IoReadWriteCallbackContext *ctx = MmAllocateKernelHeap(sizeof(*ctx));
    if(NULL == ctx)
    {
        IoFreeRp(rp);
        status = OUT_OF_RESOURCES;
        goto IoPerformReadWriteFailure;
    }
    ctx->dev = dev;
    ctx->node = node;
    ctx->alignedBuffer = alignedBuffer;
    ctx->alignedList = alignedList;
    ctx->list = list;
    ctx->size = size;
    ctx->alignedOffset = alignedOffset;
    ctx->alignedSize = alignedSize;
    ctx->offset = offset;
    ctx->callback = callback;
    ctx->context = context;
    ctx->write = write;
    ctx->useDirectIo = useDirectIo;
    ctx->fullDirect = fullDirect;
    ctx->firstWriteStep = firstWriteStep;

    if(write)
        rp->code = IO_RP_WRITE;
    else
        rp->code = IO_RP_READ;
    rp->vfsNode = node;
    rp->payload.read.offset = alignedOffset;
    rp->size = alignedSize;
    rp->completionCallback = IoReadWriteCallback;
    rp->completionContext = ctx;
    
    status = IoSendRp(dev, rp);
    if(OK == status)
        return OK;
    else
    {
        IoFreeRp(rp);
        MmFreeKernelHeap(ctx);
    }

IoPerformReadWriteFailure:
    if(!fullDirect)
    {
        if(useDirectIo)
            MmFreeMemoryDescriptorList(alignedList);
        MmFreeKernelHeap(alignedBuffer);
    }
    MmFreeMemoryDescriptorList(list);
    return status; 
}


static STATUS IoReadWriteCallback(struct IoRp *rp, void *context)
{
    STATUS status = rp->status;
    struct IoReadWriteCallbackContext *ctx = context;

    if(!ctx->firstWriteStep)
    {
        //operation was not fully direct, an intermediate buffer was used
        if(!ctx->fullDirect)
        {
            if(!ctx->write)
            {
                //map caller buffer and copy data
                void *buffer = HalMapMemoryDescriptorList(ctx->list);
                if(NULL != buffer)
                {
                    RtlMemcpy(buffer, &(ctx->alignedBuffer[ctx->offset - ctx->alignedOffset]), ctx->size);
                    HalUnmapMemoryDescriptorList(buffer);
                }
                else
                    status = OUT_OF_RESOURCES;
            }
        }
        goto IoReadWriteCallbackExit;
    }
    else
    {
        if(OK != status)
        {
            goto IoReadWriteCallbackExit;
        }
        else
        {
            //map caller buffer and copy data
            void *buffer = HalMapMemoryDescriptorList(ctx->list);
            if(NULL != buffer)
            {
                RtlMemcpy(&(ctx->alignedBuffer[ctx->offset - ctx->alignedOffset]), buffer, ctx->size);
                HalUnmapMemoryDescriptorList(buffer);
            }
            else
            {
                status = OUT_OF_RESOURCES;
                rp->size = 0;
                goto IoReadWriteCallbackExit;
            }

            //perfrom aligned write
            status = IoPerformReadWrite(true, ctx->dev, ctx->node, ctx->offset, ctx->size, ctx->alignedOffset, ctx->alignedSize,
                ctx->alignedBuffer, ctx->list, ctx->alignedList, ctx->useDirectIo, ctx->fullDirect, false, ctx->callback, ctx->context);
            if(OK != status)
            {
                goto IoReadWriteCallbackExit;
            }
            return OK;
        }
    }

IoReadWriteCallbackExit:
    if(!ctx->fullDirect)
    {
        if(ctx->useDirectIo)
        {
            MmFreeMemoryDescriptorList(ctx->alignedList);
        }

        MmFreeKernelHeap(ctx->alignedBuffer);
    }

    MmFreeMemoryDescriptorList(ctx->list);
    ctx->callback(status, ctx->size, ctx->context);

    MmFreeKernelHeap(ctx);
    return OK;
}


STATUS IoReadWrite(bool write, struct IoDeviceObject *dev, struct IoVfsNode *node, uint64_t offset, size_t size, void *buffer,
                IoReadWriteCompletionCallback callback, void *context, bool forceDirectIo)
{
    STATUS status = OK; //operation status
    struct MmMemoryDescriptor *list = NULL; //Physical Memory Descriptoor list for caller buffer
    struct MmMemoryDescriptor *alignedList = NULL; //Physical Memory Descriptor list for intermediate (aligned) buffer
    bool useDirectIo = !!(dev->flags & IO_DEVICE_FLAG_DIRECT_IO); //use direct IO flag, depends on device capabilities
    uintptr_t alignment = (dev->alignment > dev->blockSize) 
                ? dev->alignment : dev->blockSize; //required alignment: device-provided alignment requirement or block size, whichever is bigger
    size_t alignedSize = 0; //aligned buffer size
    uint64_t alignedOffset = 0; //aligned buffer offset
    uint8_t *alignedBuffer = NULL; //aligned buffer
    bool fullDirect = false; //full direct flag, that is, do not use intermediate buffering
    
    //check device capabilities, align offset and size
    if(0 == (dev->flags & (IO_DEVICE_FLAG_DIRECT_IO | IO_DEVICE_FLAG_BUFFERED_IO)))
        return OPERATION_NOT_ALLOWED;

    if(0 == alignment)
        alignment = 1;
    
    alignedOffset = ALIGN_DOWN(offset, alignment);

    if(dev->blockSize >= 1)
        alignedSize = ALIGN_UP(size + (offset - alignedOffset), dev->blockSize);

    if(forceDirectIo && !useDirectIo)
        return OPERATION_NOT_ALLOWED;

    if(forceDirectIo && ((alignedOffset != offset) || (alignedSize != size)))
        return BAD_ALIGNMENT;
    
    //everything seems to be fine
    //prepare memory descriptors for caller buffer
    //caller buffer may be context-dependent, so we must get physical pages' addresses for this buffer
    list = MmBuildMemoryDescriptorList(buffer, size);
    if(NULL == list)
        return OUT_OF_RESOURCES;
    //TODO: lock pages

    //check if direct IO is available and the provided buffer is already correctly aligned
    if(useDirectIo
        && (alignedOffset == offset) 
        && (alignedSize == size) 
        && (0 == (list[0].physical & (alignedOffset - 1))))
    {
        //if so, everything is easy, just do a direct read/write
        alignedBuffer = buffer;
        alignedList = list;
        fullDirect = true;
    }
    else
    {
        //if buffer is not aligned correctly or the device does not support direct IO,
        //we need to provide an additional aligned buffer
        fullDirect = false;
        alignedBuffer = MmAllocateKernelHeapAligned(alignedSize, alignment);
        if(NULL == alignedBuffer)
            return OUT_OF_RESOURCES;
        
        //build memory descriptor list for the aligned buffer if direct IO is used
        if(useDirectIo)
        {
            alignedList = MmBuildMemoryDescriptorList(alignedBuffer, alignedSize);
            if(NULL == alignedList)
            {
                MmFreeKernelHeap(alignedBuffer);
                MmFreeMemoryDescriptorList(list);
                return OUT_OF_RESOURCES;
            }
        }
        
        //at this point we are using the intermediate buffer either because the device does not support direct I/O
        //or the caller buffer is not aligned properly
        //in the latter case, when writing the aligned buffer, we would actually overwrite the data
        //that fell in the buffer area because of the alignment
        //then it is necessary to read data first and then replace only the part of the data,
        //and write it
        if(write)
        {
            if((alignedOffset != offset) || (alignedSize != size))
            {
                //perform aligned read
                status = IoPerformReadWrite(false, dev, node, offset, size, alignedOffset, alignedSize,
                    alignedBuffer, list, alignedList, useDirectIo, fullDirect, true, callback, context);
                return status;
            }
            else
            {
                RtlMemcpy(alignedBuffer, buffer, size);
            }
        }
    }

    status = IoPerformReadWrite(write, dev, node, offset, size, alignedOffset, alignedSize,
        alignedBuffer, list, alignedList, useDirectIo, fullDirect, false, callback, context);
    return status;
}

STATUS IoReadDeviceSync(struct IoDeviceObject *dev, uint64_t offset, size_t size, void **buffer)
{
    STATUS status = OK;
    struct MmMemoryDescriptor *list = NULL;
    struct IoRp *rp = NULL;
    bool useDirectIo = !!(dev->flags & IO_DEVICE_FLAG_DIRECT_IO);
    uintptr_t alignment = (dev->alignment > dev->blockSize) ? dev->alignment : dev->blockSize;
    size_t alignedSize = 0;
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

            RtlMemcpy(*buffer, &(alignedBuffer[offset - alignedOffset]), size);
            MmFreeKernelHeap(alignedBuffer);
        }
    }

_IoDeviceReadSyncExit:
    IoFreeRp(rp);
    MmFreeMemoryDescriptorList(list);
    return status;
}