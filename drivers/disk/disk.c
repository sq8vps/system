#include "disk.h"
#include "mm/palloc.h"

static STATUS DiskVerifyMemoryTable(struct MmMemoryDescriptor *mem, uint64_t align, uint64_t requiredSize, uint64_t blockSize)
{
    // if(align <= 1)
    //     return OK;
    
    // if(requiredSize % blockSize)
    //     return BAD_ALIGNMENT;

    // uint64_t size = 0;

    // uint64_t mask = ((uint64_t)1 << (align - 1)) - 1;
    // while(NULL != mem)
    // {
    //     if((mem->physical & mask) || (mem->size & mask))
    //         return BAD_ALIGNMENT;
    //     size += mem->size;
    //     mem = mem->next;
    // }
    
    // if(size < requiredSize)
    //     return OUT_OF_RESOURCES;

    // return OK;
}

static STATUS DiskFinalizeTransaction(struct IoRp *rp, void *context)
{
    IoFinalizeRp((struct IoRp*)context);
    return OK;
}

static STATUS DiskPerformTransaction(struct IoRp *original)
{
    // STATUS status;
    // struct IoRp *rp;
    // status = IoCreateRp(&rp);
    // if(OK != status)
    //     return status;
    // rp->code = original->code;
    // rp->device = original->device->attachedTo;
    // //rp->payload.readWrite = original->payload.readWrite;
    // rp->size = original->size;
    // rp->completionCallback = DiskFinalizeTransaction;
    // rp->completionContext = original;
    // return IoSendRp(rp->device, rp);
}

STATUS DiskReadWrite(struct IoRp *rp)
{

}