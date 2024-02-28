#include "part.h"
#include "io/dev/rp.h"
#include "mm/palloc.h"
#include "mm/valloc.h"
#include "mm/heap.h"
#include "mm/mmio.h"
#include "logging.h"

static STATUS PartmgrInitializeCallback(struct IoRp *rp, void *context)
{
    struct PartmgrDriveData *info = context;

    if(OK == rp->status)
    {
        void *m = MmMapMmIo(rp->payload.readWrite.memory->physical, rp->payload.readWrite.memory->size);
        if(NULL != m)
        {
            info->mbr = MmAllocateKernelHeap(sizeof(*(info->mbr)));
            if(NULL != info->mbr)
            {
                info->scheme = SCHEME_MBR;
                PartmgrMbrParse(m, info->mbr);
            }
            MmUnmapMmIo(m);
        }
        MmFreePhysicalMemory(rp->payload.readWrite.memory->physical, rp->payload.readWrite.memory->size);
    }
    
    return OK;
}

STATUS PartmgrInitialize(struct PartmgrDriveData *info)
{
    STATUS status;

    if(!info->ioParams.read.direct.available)
        return OK;
    
    //read MBR and GPT header at once
    //MBT header is in LBA 0, GPT header is in LBA 1, so we need to read two sectors.
    //The address in bytes depends on sector size
    
    uintptr_t addr;
    uint64_t size = MmAllocateContiguousPhysicalMemory(
        info->ioParams.read.direct.blockSize * 2, 
        &addr, 
        info->ioParams.read.direct.requiredAlignment);
    if(0 == size)
        return OUT_OF_RESOURCES;

    struct IoRp *rp;
    status = IoCreateRp(&rp);
    if(OK != status)
    {
        MmFreePhysicalMemory(addr, size);
        return status;
    }

    struct IoMemoryDescriptor *mem = MmAllocateKernelHeap(sizeof(*mem));
    if(NULL == mem)
    {
        MmFreePhysicalMemory(addr, size);
        MmFreeKernelHeap(rp);
        return OUT_OF_RESOURCES;
    }
    mem->next = NULL;
    mem->mapped = NULL;
    mem->physical = addr;
    mem->size = size;
    
    //read
    rp->device = info->device->attachedTo;
    rp->flags = 0;
    rp->code = IO_RP_READ;
    rp->completionCallback = PartmgrInitializeCallback;
    rp->completionContext = info;
    rp->size = info->ioParams.read.direct.blockSize * 2;
    rp->payload.readWrite.offset = 0;
    rp->payload.readWrite.memory = mem;

    status = IoSendRp(info->device->attachedTo, rp);
    if(OK != status)
    {
        MmFreePhysicalMemory(addr, size);
        MmFreeKernelHeap(rp);
        MmFreeKernelHeap(mem);
        return status;
    }
    return OK;
}