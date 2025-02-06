#include "read.h"
#include "fat.h"
#include "io/dev/rp.h"
#include "io/fs/vfs.h"
#include "io/dev/op.h"
#include "io/dev/dev.h"
#include "structs.h"
#include "mm/heap.h"
#include "mm/mm.h"
#include "fsctrl.h"
#include <stdbool.h>

struct FatReadCallbackContext
{
    struct FatVolume *vol;
    struct IoRp *originalRp;
    uint64_t toTransfer;
    uint64_t remaining;
    uint32_t cluster;
    bool write;
};

static STATUS FatReadWriteCallback(struct IoRp *rp, void *context)
{
    struct FatReadCallbackContext *ctx = context;
    STATUS status = rp->status;
    if(rp->size != ctx->toTransfer)
        status = ctx->write ? WRITE_INCOMPLETE : READ_INCOMPLETE;
    
    if(OK != status)
        goto FatReadCallbackExit;

    ctx->remaining -= rp->size;

    uint32_t clusters = rp->size / (ctx->vol->sectorsPerCluster * ctx->vol->disk->blockSize)
        + ((rp->size % (ctx->vol->sectorsPerCluster * ctx->vol->disk->blockSize)) ? 1 : 0) - 1;

    if(FatIsClusterEof(ctx->vol, ctx->cluster) || (0 == ctx->remaining))
    {
        if(0 != ctx->remaining)
            ctx->originalRp->flags |= IO_RP_FLAG_EOF;
        goto FatReadCallbackExit;
    } 

    ctx->cluster = FatGetNextCluster(ctx->vol, ctx->cluster + clusters);

    if(!FAT_CLUSTER_VALID(ctx->vol, ctx->cluster))
    {
        status = FILE_BROKEN;
        goto FatReadCallbackExit;
    }

    rp = IoCloneRp(rp);
    if(NULL == rp)
    {
        status = OUT_OF_RESOURCES;
        goto FatReadCallbackExit;
    }

    if((NULL != rp->payload.read.memory) || (NULL != rp->payload.write.memory))
    {
        uint64_t sum = 0;
        struct MmMemoryDescriptor *d;
        if(ctx->write)
            d = rp->payload.write.memory;
        else
            d = rp->payload.read.memory;
        while(NULL != d)
        {
            if((sum + d->size) > rp->size)
            {
                d->physical += (rp->size - sum);
                d->size -= (rp->size - sum);
                break;
            }
            sum += d->size;
            if(ctx->write)
                rp->payload.write.memory = d->next;
            else
                rp->payload.read.memory = d->next;
            MmFreeMemoryDescriptor(d);
            d = ctx->write ? rp->payload.write.memory : rp->payload.read.memory;
        }
    }
    else if(!ctx->write)
        rp->payload.read.systemBuffer = (void*)((uintptr_t)rp->payload.read.systemBuffer + (uintptr_t)rp->size);
    else
        rp->payload.write.systemBuffer = (void*)((uintptr_t)rp->payload.write.systemBuffer + (uintptr_t)rp->size);

    if(ctx->write)
        rp->payload.write.offset = FAT_GET_OFFSET(ctx->vol, ctx->cluster);
    else
        rp->payload.read.offset = FAT_GET_OFFSET(ctx->vol, ctx->cluster);

    //get number of consecutive clusters
    uint64_t consecutive = FatGetConsecutiveClusterCount(ctx->vol, ctx->cluster) 
        * ctx->vol->sectorsPerCluster * ctx->vol->disk->blockSize;
    //read as many bytes at once
    if(consecutive <= ctx->remaining)
        rp->size = consecutive;
    else
        rp->size = ctx->remaining;
    
    ctx->toTransfer = rp->size;

    status = IoSendRp(ctx->vol->disk, rp);
    if(OK == status)
        return OK;
    else
        ctx->originalRp->status = status;

FatReadCallbackExit:
    FatUpdateFileAttributes(ctx->vol, ctx->originalRp->vfsNode);

    ctx->originalRp->size -= ctx->remaining;
    ctx->originalRp->status = status;
    IoFinalizeRp(ctx->originalRp);
    if(!ctx->write && (NULL != rp->payload.read.memory))
        MmFreeMemoryDescriptorList(rp->payload.read.memory);
    else if(ctx->write && (NULL != rp->payload.write.memory))
        MmFreeMemoryDescriptorList(rp->payload.write.memory);

    //update FAT if necessary
    FatUpdateOnDisk(ctx->vol);
    MmFreeKernelHeap(ctx);
    return status;
}

STATUS FatReadWrite(struct IoRp *rp)
{
    bool write = (IO_RP_READ == rp->code) ? false : true;
    bool eof = false;

    if((IO_RP_READ != rp->code) && (IO_RP_WRITE != rp->code))
        return RP_PROCESSING_FAILED;
    
    if(NULL == rp->vfsNode)
        return FILE_NOT_FOUND;
    
    if(!((IO_VFS_FILE == rp->vfsNode->type) || (IO_VFS_LINK == rp->vfsNode->type)))
        return BAD_FILE_TYPE;
    
    if(rp->vfsNode->device != rp->device)
        return RP_PROCESSING_FAILED;
    
    struct FatVolume *vol = rp->device->privateData;
    if(NULL == vol)
        return RP_PROCESSING_FAILED;
    
    if(!write)
    {
        if(rp->payload.read.offset >= rp->vfsNode->size)
        {
            //nothing to read
            rp->size = 0;
            rp->status = OK;
            IoFinalizeRp(rp);
            return OK;
        }
    }
    else //write mode
    {
        if(rp->payload.write.offset > rp->vfsNode->size)
        {
            return FILE_TOO_SMALL;
        }
    }

    //clone RP, because we need to read in parts - the file may be fragmented
    struct IoRp *n = IoCloneRp(rp);
    if(NULL == n)
        return OUT_OF_RESOURCES;

    struct FatReadCallbackContext *ctx = MmAllocateKernelHeap(sizeof(*ctx));
    if(NULL == ctx)
    {
        IoFreeRp(n);
        return OUT_OF_RESOURCES;
    }

    //calculate cluster number including offset
    uint32_t clusterShift = (write ? n->payload.write.offset : n->payload.read.offset)
         / (vol->sectorsPerCluster * vol->disk->blockSize);
    ctx->cluster = n->vfsNode->ref[1].u32;
    for(uint32_t i = 0; i < clusterShift; i++)
    {
        uint32_t next = FatGetNextCluster(vol, ctx->cluster);
        if(!FAT_CLUSTER_VALID(vol, next))
        {
            if(FatIsClusterEof(vol, next))
                eof = true;
            else
                ctx->cluster = next;
            break;
        }
        ctx->cluster = next;
    }

    if(!FAT_CLUSTER_VALID(vol, ctx->cluster))
    {
        MmFreeKernelHeap(ctx);
        IoFreeRp(n);
        return FILE_BROKEN;
    }

    uint64_t consecutive = 0; //number of available consecutive bytes

    if(write)
    {
        //the cluster number calculated above assumes that the offset is within the cluster
        //already occupied by the file
        //however, there is an corner case, where the offset is equal to the cluster size multiple,
        //that is, when the file occupies an integer multiple of cluster size and we are appending data to it
        //in such a case, a new cluster must be found
        //also, we can reserve as many consecutive clusters as required and possible to 
        //speed up the process
        uint32_t requiredClusters = 0;
        if((rp->vfsNode->size == rp->payload.write.offset) 
            && (0 == (rp->payload.write.offset % (vol->sectorsPerCluster * vol->disk->blockSize))))
        {
            //handle corner case
            requiredClusters = rp->size / (vol->sectorsPerCluster * vol->disk->blockSize)
                + ((rp->size % (vol->sectorsPerCluster * vol->disk->blockSize)) ? 1 : 0);
        }
        else
        {
            eof = false;
            //otherwise there is some space remaining in the existing cluster
            uint64_t remaining = (vol->sectorsPerCluster * vol->disk->blockSize) 
                - (rp->vfsNode->size % (vol->sectorsPerCluster * vol->disk->blockSize));
            requiredClusters = (rp->size - remaining) / (vol->sectorsPerCluster * vol->disk->blockSize)
                + (((rp->size - remaining) % (vol->sectorsPerCluster * vol->disk->blockSize)) ? 1 : 0);
        }

        if(0 != requiredClusters)
        {
            PRIO prio = KeAcquireSpinlock(&(vol->fatLock));
            //reserve clusters
            if(0 != FatReserveClusters(vol, ctx->cluster, requiredClusters))
            {
                FatFreeClusters(vol, ctx->cluster);
                MmFreeKernelHeap(ctx);
                KeReleaseSpinlock(&(vol->fatLock), prio);
                IoFreeRp(n);
                return OUT_OF_RESOURCES; 
            }
            if(eof)
                ctx->cluster = FatGetNextCluster(vol, ctx->cluster);
            KeReleaseSpinlock(&(vol->fatLock), prio);
        }
    }    
    
    
    //get number of consecutive bytes
    consecutive = FatGetConsecutiveClusterCount(vol, ctx->cluster) * vol->sectorsPerCluster * vol->disk->blockSize;

    if(write)
    {
        n->payload.write.offset -= (clusterShift * vol->sectorsPerCluster * vol->disk->blockSize);
        consecutive -= n->payload.write.offset;
        n->payload.write.offset += FAT_GET_OFFSET(vol, ctx->cluster);
    }
    else
    {
        n->payload.read.offset -= (clusterShift * vol->sectorsPerCluster * vol->disk->blockSize);
        consecutive -= n->payload.read.offset;
        n->payload.read.offset += FAT_GET_OFFSET(vol, ctx->cluster);
    }
    
    ctx->remaining = rp->size;

    //transfer as many bytes at once
    if(consecutive <= ctx->remaining)
        n->size = consecutive;
    else
        n->size = ctx->remaining;

    ctx->toTransfer = n->size;
    ctx->write = write;

    n->completionCallback = FatReadWriteCallback;
    n->completionContext = ctx;

    if(!write && (NULL != rp->payload.read.memory))
    {
        n->payload.read.memory = MmCloneMemoryDescriptorList(rp->payload.read.memory);
        if(NULL == n->payload.read.memory)
        {
            MmFreeKernelHeap(ctx);
            IoFreeRp(n);
            return OUT_OF_RESOURCES;
        }
    }
    else if(write && (NULL != rp->payload.write.memory))
    {
        n->payload.write.memory = MmCloneMemoryDescriptorList(rp->payload.write.memory);
        if(NULL == n->payload.write.memory)
        {
            MmFreeKernelHeap(ctx);
            IoFreeRp(n);
            return OUT_OF_RESOURCES;
        }
    }

    ctx->vol = vol;
    ctx->originalRp = rp;

    IoMarkRpPending(rp);

    STATUS status = IoSendRp(vol->disk, n);
    if(OK != status)
    {
        if(!write && (NULL != n->payload.read.memory))
            MmFreeMemoryDescriptorList(n->payload.read.memory);
        else if(write && (NULL != n->payload.write.memory))
            MmFreeMemoryDescriptorList(n->payload.write.memory);
        MmFreeKernelHeap(ctx);
        IoFreeRp(n);
        rp->status = status;
        IoFinalizeRp(rp);
    }
    return status;
}