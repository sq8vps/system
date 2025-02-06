#include "fat.h"
#include "structs.h"
#include "io/dev/op.h"
#include "io/dev/dev.h"
#include "mm/heap.h"
#include "io/fs/vfs.h"
#include "io/dev/rp.h"
#include "utils.h"
#include "ddk/fs.h"
#include "rtl/order.h"
#include "logging.h"

struct FatGetEntryContext
{
    struct IoRp *rp;
    struct FatVolume *vol;
    struct FatDirectory *list;
    union FsGetRequest *req;
    uint32_t cluster;
    uint32_t size;
    char lastFileName[512];
};

uint32_t FatGetNextCluster(struct FatVolume *vol, uint32_t currentCluster)
{
    uint32_t c = 0;
    switch(vol->type)
    {
        case FAT16:
            c = RtlLeU16(((uint16_t*)vol->fat)[currentCluster]);
            break;
        case FAT32:
            c = RtlLeU32(((uint32_t*)vol->fat)[currentCluster]) & 0xFFFFFFF;
            break;
        case FAT12:
            //TODO: untested for FAT12
            if(currentCluster & 1) //odd cluster number
            {
                c = ((uint8_t*)vol->fat)[((currentCluster - 1) / 2) * 3] >> 4;
                c |= ((uint16_t)((uint8_t*)vol->fat)[((currentCluster - 1) / 2) * 3 + 1] << 4);
                c &= 0xFFF;
            }
            else //even cluster number
            {
                c = ((uint8_t*)vol->fat)[(currentCluster / 2) * 3];
                c |= ((uint16_t)((uint8_t*)vol->fat)[(currentCluster / 2) * 3 + 1] << 8);
                c &= 0xFFF;
            }
            break;
    }
    return c;
}

bool FatIsClusterEof(struct FatVolume *vol, uint32_t cluster)
{
    uint32_t val = FatGetNextCluster(vol, cluster);
    switch(vol->type)
    {
        case FAT16:
            return (RtlLeU16(val) == 0xFFFF);
            break;
        case FAT32:
            return (RtlLeU32(val) == 0xFFFFFFF);
            break;
        case FAT12:
            return (RtlLeU16(val) == 0xFFF);
            break;
    }
    return false;
}

void FatWriteNextCluster(struct FatVolume *vol, uint32_t currentCluster, uint32_t nextCluster)
{
    if(currentCluster < vol->modifiedClusterLow)
        vol->modifiedClusterLow = currentCluster;
    if(currentCluster > vol->modifiedClusterHigh)
        vol->modifiedClusterHigh = currentCluster;
    switch(vol->type)
    {
        case FAT16:
            ((uint16_t*)vol->fat)[currentCluster] = RtlLeU16(nextCluster);
            break;
        case FAT32:
            ((uint32_t*)vol->fat)[currentCluster] &= 0xF0000000;
            ((uint32_t*)vol->fat)[currentCluster] |= (RtlLeU32(nextCluster) & 0xFFFFFFF);
            break;
        case FAT12:
            if(currentCluster & 1) //odd cluster number
            {
                ((uint8_t*)vol->fat)[((currentCluster - 1) / 2) * 3] &= 0x0F;
                ((uint8_t*)vol->fat)[((currentCluster - 1) / 2) * 3] |= ((nextCluster & 0xF) << 4);
                ((uint8_t*)vol->fat)[((currentCluster - 1) / 2) * 3 + 1] = (nextCluster >> 4);
            }
            else //even cluster number
            {
                ((uint8_t*)vol->fat)[(currentCluster / 2) * 3] = nextCluster & 0xFF;
                ((uint8_t*)vol->fat)[(currentCluster / 2) * 3 + 1] &= 0xF0;
                ((uint8_t*)vol->fat)[(currentCluster / 2) * 3 + 1] |= ((nextCluster >> 8) & 0xF);
            }
            break;
    }    
}

uint32_t FatGetConsecutiveClusterCount(struct FatVolume *vol, uint32_t cluster)
{
    uint32_t count = 0;
    uint32_t newCluster = cluster;
    do
    {
        cluster = newCluster;
        newCluster = FatGetNextCluster(vol, cluster);
        if(!FAT_CLUSTER_VALID(vol, newCluster))
            return count + 1;
        count++;
    }
    while(newCluster == (cluster + 1));
    return count;
}

int FatReserveClusters(struct FatVolume *vol, uint32_t cluster, uint32_t count)
{
    uint32_t current = 2;
    while(count && (current < vol->clusters))
    {
        if(FAT_CLUSTER_FREE(FatGetNextCluster(vol, current)))
        {
            FatWriteNextCluster(vol, cluster, current);
            FatWriteNextCluster(vol, current, FAT_CLUSTER_EOF);
            cluster = current;
            count--;
        }
        current++;
    }
    return count;
}

void FatFreeClusters(struct FatVolume *vol, uint32_t cluster)
{
    uint32_t c = FatGetNextCluster(vol, cluster);
    FatWriteNextCluster(vol, cluster, FAT_CLUSTER_EOF);
    while(!FatIsClusterEof(vol, c))
    {
        c = FatGetNextCluster(vol, c);
    }
    FatWriteNextCluster(vol, c, 0);
}

static void FatFillVfsNode(struct IoVfsNode *n, struct FatDirectory *e, struct FatVolume *vol)
{
    //store cluster for file data start
    n->ref[1].u32 = ((uint32_t)RtlLeU16(e->fstClusLo) | ((uint32_t)RtlLeU16(e->fstClusHi) << 16));
    n->device = IoGetDeviceStackTop(vol->vol);
    n->fsType = IO_VFS_FS_PHYSICAL;
    n->type = (e->attributes & FAT_ATTR_DIRECTORY) ? IO_VFS_DIRECTORY : IO_VFS_FILE;
    n->flags |= (e->attributes & (FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM)) ? IO_VFS_FLAG_HIDDEN : 0;
    n->size = RtlLeU32(e->fileSize);
    //TODO: implement dates handling
}

static void FatGetEntryCallback(STATUS status, size_t actualSize, void *context)
{
    struct FatGetEntryContext *ctx = context;
    if(OK == status)
    {
        uint32_t i = 0;
        while((FAT_DIRECTORY_ENTRY_EMPTY_LAST != ctx->list[i].name[0])
            && ((uintptr_t)&(ctx->list[i]) - (uintptr_t)&(ctx->list[0])) < actualSize)
        {
            //0xE5 is used for deleted entries in general
            //skip such entries
            if(0xE5 == (unsigned char)ctx->list[i].name[0])
            {
                goto FatGetEntryCallbackContinue;
            }

            //long file name entry
            if(FAT_ATTR_LONG_NAME == (ctx->list[i].attributes & FAT_ATTR_LONG_NAME_MASK))
            {
                struct FatLongFileNameEntry *lfn = (struct FatLongFileNameEntry*)&(ctx->list[i]);
                //store last long file name to avoid additional LFN lookups
                uint32_t offset = lfn->ord & FAT_DIRECTORY_ENTRY_LONG_NAME_ENTRY_MASK;
                if((offset < 1) || (offset > 20))
                {
                    //entry is broken
                    goto FatGetEntryCallbackContinue;
                }
                offset = (offset - 1) * 26; //13 UCS-2 characters per entry
                RtlMemcpy(&(ctx->lastFileName[offset]), lfn->name1, sizeof(lfn->name1));
                RtlMemcpy(&(ctx->lastFileName[offset + sizeof(lfn->name1)]), lfn->name2, sizeof(lfn->name2));
                RtlMemcpy(&(ctx->lastFileName[offset + sizeof(lfn->name1) + sizeof(lfn->name2)]), lfn->name3, sizeof(lfn->name3));
                i++;
                continue;
            }
            
            //skip volume ids
            if(ctx->list[i].attributes & FAT_ATTR_VOLUME_ID)
            {
                goto FatGetEntryCallbackContinue;
            }


            //otherwise it is a correct entry

            if(FS_GET_NODE == ctx->rp->payload.deviceControl.code)
            {
                //check if there was a LFN for this entry
                if((0 != ctx->lastFileName[0]) || (0 != ctx->lastFileName[1]))
                {
                    if(0 != FatCompareUcs2AndUtf8(ctx->lastFileName, ctx->req->getNode.name))
                    {
                        goto FatGetEntryCallbackContinue;
                    }
                }
                else //no LFN entry
                {
                    char dosName[12];
                    if(0 != FatFileNameToDosName(ctx->req->getNode.name, dosName))
                    {
                        goto FatGetEntryCallbackContinue;
                    }
                    if(0 != RtlStrncmp(dosName, ctx->list[i].name, 11))
                    {
                        goto FatGetEntryCallbackContinue;
                    }
                }

                ctx->req->getNode.node = IoVfsCreateNode(ctx->req->getNode.name);
                if(NULL != ctx->req->getNode.node)
                {
                    //store file header location
                    ctx->req->getNode.node->ref[0].u64 = FAT_GET_OFFSET(ctx->vol, ctx->cluster) 
                        + (uintptr_t)&(ctx->list[i]) - (uintptr_t)&(ctx->list[0]);
                    FatFillVfsNode(ctx->req->getNode.node, &(ctx->list[i]), ctx->vol);
                    ctx->rp->status = OK;
                }
                else
                    ctx->rp->status = OUT_OF_RESOURCES;
            
                IoFinalizeRp(ctx->rp);
                MmFreeKernelHeap(ctx);
                return;
            }
            else //FS_GET_NODE_CHILDREN
            {
                char *name = MmAllocateKernelHeap(IoVfsGetMaxFileNameLength() + 1);
                if(NULL != name)
                {
                    if((0 != ctx->lastFileName[0]) || (0 != ctx->lastFileName[1])) //long file name
                    {
                        if(0 != FatUcs2ToUtf8(name, ctx->lastFileName, IoVfsGetMaxFileNameLength()))
                            status = ILLEGAL_NAME;
                    }
                    else //8.3 file name
                    {
                        if(0 != FatDosNameToFileName(name, ctx->list[i].name))
                            status = ILLEGAL_NAME;
                    }

                    if(OK == status)
                    {
                        struct IoVfsNode *n = IoVfsCreateNode(name);
                        if(NULL != n)
                        {
                            //store file header location
                            ctx->req->getNode.node->ref[0].u64 = FAT_GET_OFFSET(ctx->vol, ctx->cluster) 
                                + (uintptr_t)&(ctx->list[i]) - (uintptr_t)&(ctx->list[0]);
                            FatFillVfsNode(n, &(ctx->list[i]), ctx->vol);
                            
                            if(NULL == ctx->req->getChildren.children)
                                ctx->req->getChildren.children = n;
                            else
                            {
                                struct IoVfsNode *t = ctx->req->getChildren.children;
                                while(NULL != t->next)
                                {
                                    t = t->next;
                                }
                                t->next = n;
                                n->previous = t;
                            }
                        }
                    }
                    else
                        ctx->rp->status = OUT_OF_RESOURCES;
                    
                    MmFreeKernelHeap(name);
                }
                else
                    status = OUT_OF_RESOURCES;

                if(OK != status)
                {
                    struct IoVfsNode *t = ctx->req->getChildren.children;
                    while(NULL != t->next)
                    {
                        struct IoVfsNode *k = t->next;
                        IoVfsDestroyNode(t);
                        t = k;
                    }
                    IoFinalizeRp(ctx->rp);
                    MmFreeKernelHeap(ctx);
                    return;
                }
            }
FatGetEntryCallbackContinue:
            ctx->lastFileName[0] = 0;
            ctx->lastFileName[1] = 0;
            i++;
        }
        //end of cluster or no more entries
        if(0 == ctx->cluster)
        {
            //special case - cluster number was 0, that is, we were reading root directory in FAT16/12
            if(FS_GET_NODE == ctx->rp->payload.deviceControl.code)
                ctx->rp->status = FILE_NOT_FOUND;
            else
                ctx->rp->status = OK;
            IoFinalizeRp(ctx->rp);
            MmFreeKernelHeap(ctx);
            return;
        }
        ctx->cluster = FatGetNextCluster(ctx->vol, ctx->cluster);
        //check if next cluster is within a valid range
        if(FAT_CLUSTER_VALID(ctx->vol, ctx->cluster))
        {
            status = IoReadWrite(false, ctx->vol->disk, NULL, FAT_GET_OFFSET(ctx->vol, ctx->cluster), 
                ctx->size, ctx->list, FatGetEntryCallback, ctx, false);
            if(OK != status)
            {
                ctx->rp->status = status;
                IoFinalizeRp(ctx->rp);
                MmFreeKernelHeap(ctx);
                return;
            }
        }
    }
    else
    {
        ctx->rp->status = status;
        IoFinalizeRp(ctx->rp);
        MmFreeKernelHeap(ctx);
        return;
    }

    if(FS_GET_NODE == ctx->rp->payload.deviceControl.code)
        ctx->rp->status = FILE_NOT_FOUND;
    else
        ctx->rp->status = OK;

    IoFinalizeRp(ctx->rp);
    MmFreeKernelHeap(ctx->list);
    MmFreeKernelHeap(ctx);
}

STATUS FatGetNode(struct IoRp *rp, struct FatVolume *vol)
{
    uint32_t cluster = 0;
    uint64_t offset = 0;
    uint64_t size = 0;

    union FsGetRequest *req = rp->payload.deviceControl.data;
    if(FS_GET_NODE == rp->payload.deviceControl.code)
    {
        //check if parent is specified
        if((NULL != req->getNode.parent) && (IO_VFS_MOUNT_POINT != req->getNode.parent->type))
        {
            //if so, then set offset appropriately
            offset = FAT_GET_OFFSET(vol, req->getNode.parent->ref[1].u32);
            cluster = req->getNode.parent->ref[1].u32;
        }

    }
    else
    {
        if((NULL != req->getChildren.node) && (IO_VFS_MOUNT_POINT != req->getChildren.node->type))
        {
            offset = FAT_GET_OFFSET(vol, req->getNode.parent->ref[1].u32);
            cluster = req->getNode.parent->ref[1].u32;
        }
    }

    if(0 == cluster)
    {
        if(FAT32 == vol->type)
        {
            cluster = vol->rootCluster;
            offset = FAT_GET_OFFSET(vol, cluster);
        }
        else
            offset = FAT_ROOT_OFFSET(vol);
    }

    //read whole cluster if FAT32 or if not FAT32, but it is not the root directory
    if((FAT32 == vol->type) || (0 != cluster))
        size = vol->sectorsPerCluster * vol->disk->blockSize;
    else //otherwise this should be the root directory in FAT16/12
        size = vol->rootEntryCount * 32; //entry size = 32, resulting value should be a multiple of sector size

    struct FatDirectory *list = MmAllocateKernelHeapAligned(size, IO_DEV_REQUIRED_ALIGNMENT(vol->disk));
    if(NULL == list)
        return OUT_OF_RESOURCES;

    struct FatGetEntryContext *ctx = MmAllocateKernelHeap(sizeof(*ctx));
    if(NULL == ctx)
    {
        MmFreeKernelHeap(list);
        return OUT_OF_RESOURCES;
    }

    ctx->rp = rp;
    ctx->vol = vol;
    ctx->list = list;
    ctx->lastFileName[0] = 0;
    ctx->lastFileName[1] = 0;
    ctx->req = req;
    ctx->cluster = cluster;
    ctx->size = size;
    IoMarkRpPending(rp);

    return IoReadWrite(false, vol->disk, NULL, offset, size, list, FatGetEntryCallback, ctx, false);
}

static void FatUpdateOnDiskCallback(STATUS status, size_t actualSize, void *context)
{
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Writing FAT #%d failed with status 0x%X", (int)context, status);
    }
}

void FatUpdateOnDisk(struct FatVolume *vol)
{
    STATUS status = OK;
    PRIO prio = KeAcquireSpinlock(&(vol->fatLock));
    if(vol->modifiedClusterLow <= vol->modifiedClusterHigh)
    {
        uint64_t lowShift = 0, highShift = 0;
        switch(vol->type)
        {
            case FAT16:
                lowShift = vol->modifiedClusterLow * 2;
                highShift = vol->modifiedClusterHigh * 2;
                break;
            case FAT32:
                lowShift = vol->modifiedClusterLow * 4;
                highShift = vol->modifiedClusterHigh * 4;
                break;
            case FAT12:
                lowShift = (3 * vol->modifiedClusterLow) / 2;
                highShift = (3 * vol->modifiedClusterHigh) / 2;
                break;
        }
        vol->modifiedClusterHigh = 0;
        vol->modifiedClusterLow = UINT32_MAX;
        KeReleaseSpinlock(&(vol->fatLock), prio);
        lowShift = ALIGN_DOWN(lowShift, vol->disk->blockSize);
        highShift = ALIGN_UP(highShift, vol->disk->blockSize);
        for(uint8_t i = 0; i < vol->fatCount; i++)
        {
            status = IoReadWrite(true, vol->disk, NULL, 
                (vol->reservedSectors + i * vol->fatSize) * vol->disk->blockSize, highShift - lowShift, 
                (char*)vol->fat + lowShift, FatUpdateOnDiskCallback, (void*)(i + 1), false);
            if(OK != status)
                LOG(SYSLOG_ERROR, "Writing FAT #%d failed with status 0x%X", (int)(i + 1), status);
        }
        return;
    }
    KeReleaseSpinlock(&(vol->fatLock), prio);
}