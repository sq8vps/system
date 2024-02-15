#include "disk.h"
#include "io/dev/types.h"
#include "mm/palloc.h"

static STATUS DiskVerifyMemoryTable(struct IoMemoryDescriptor *mem, uint64_t align, uint64_t requiredSize, uint64_t blockSize)
{
    if(align <= 1)
        return OK;
    
    if(requiredSize % blockSize)
        return BAD_ALIGNMENT;

    uint64_t size = 0;

    uint64_t mask = ((uint64_t)1 << (align - 1)) - 1;
    while(NULL != mem)
    {
        if((mem->physical & mask) || (mem->size & mask))
            return BAD_ALIGNMENT;
        size += mem->size;
        mem = mem->next;
    }
    
    if(size < requiredSize)
        return OUT_OF_RESOURCES;

    return OK;
}

static STATUS DiskFinalizeTransaction(struct IoDriverRp *rp, void *context)
{
    IoFinalizeRp((struct IoDriverRp*)context);
    return OK;
}

static STATUS DiskPerformTransaction(struct IoDriverRp *original)
{
    STATUS status;
    struct IoDriverRp *rp;
    status = IoCreateRp(&rp);
    if(OK != status)
        return status;
    rp->code = original->code;
    rp->device = original->device->attachedTo;
    rp->payload.readWrite = original->payload.readWrite;
    rp->size = original->size;
    rp->completionCallback = DiskFinalizeTransaction;
    rp->completionContext = original;
    return IoSendRp(rp->device, NULL, rp);
}

STATUS DiskReadWrite(struct IoDriverRp *rp)
{
    STATUS status;
    if(IoGetCurrentRpPosition(rp) != rp->device)
        return OPERATION_NOT_ALLOWED;
    
    //get disk drive information structure
    struct DiskData *info = rp->device->privateData;
    if((NULL == info) || (!info->usable))
        return DEVICE_NOT_AVAILABLE;


    /*
        There are multiple scenarios of direct and buffered I/O combinations:
        1. Direct I/O is requested and the drive supports direct I/O
            a) If memory alignment is correct, then do full direct transfer
            b) If memory alignment is not correct, fail
        2. Direct I/O is requested and the drive supports buffered I/O
            There is no guarantee that the memory for direct I/O occupies an integral number of pages,
            thus there is no guarantee that the memory can be mapped directly to virtual addresses.
            Create local logically contiguous buffer and use it as an intermediate layer.
        3. Buffered I/O is requested and the drive supports buffered I/O
            No problem here...
        4. Buffered I/O is requested and the drive supports direct I/O
            Get physical representation of the buffer and use it for direct I/O. 
            There might be a problem with alignment - 
    */

    //verify if request can be satisfied
    if(IO_RP_READ == rp->code)
    {
        if(0 == rp->size)
        {
            rp->status = OK;
            IoFinalizeRp(rp);
            return OK;
        }

        if(info->ioParams.read.direct.available) //prefer direct I/O on the drive side
        {
            if(NULL != rp->payload.readWrite.memory) //memory descriptor provided for direct I/O?
            {
                //do full direct I/O, but alignment might be the problem
                if(rp->payload.readWrite.offset % info->ioParams.read.direct.blockSize)
                    return BAD_ALIGNMENT;
                if(OK != (
                    status = DiskVerifyMemoryTable(rp->payload.readWrite.memory, info->ioParams.read.direct.requiredAlignment, 
                            rp->size, info->ioParams.read.direct.blockSize)))
                    return status;
                
                return DiskPerformTransaction(rp);
            }
            else if(NULL != rp->payload.readWrite.systemBuffer) //buffer provided?
            {
                
            }
            else
                return NULL_POINTER_GIVEN; //no memory description was given
        }
        else if(info->ioParams.read.buffered.available)
        {
            
        }
        else
            return OPERATION_NOT_ALLOWED;
    }
    else if(IO_RP_WRITE == rp->code)
    {
        if(0 == rp->size)
        {
            rp->status = OK;
            IoFinalizeRp(rp);
            return OK;
        }

        if(info->ioParams.write.direct.available) //prefer direct I/O on the drive side
        {
            if(NULL != rp->payload.readWrite.memory) //memory descriptor provided for direct I/O?
            {
                //do full direct I/O, but alignment might be the problem
                if(rp->payload.readWrite.offset % info->ioParams.read.direct.blockSize)
                    return BAD_ALIGNMENT;
                if(OK != (
                    status = DiskVerifyMemoryTable(rp->payload.readWrite.memory, info->ioParams.write.direct.requiredAlignment, 
                            rp->size, info->ioParams.write.direct.blockSize)))
                    return status;
                
                return DiskPerformTransaction(rp);
            }
            else if(NULL != rp->payload.readWrite.systemBuffer) //buffer provided?
            {
                
            }
            else
                return NULL_POINTER_GIVEN; //no memory description was given
        }
        else if(info->ioParams.read.buffered.available)
        {
            
        }
        else
            return OPERATION_NOT_ALLOWED;
    }

    return OPERATION_NOT_ALLOWED;
}