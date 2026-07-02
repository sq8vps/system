#include "fat.h"
#include "structs.h"
#include "io/dev/op.h"
#include "io/dev/dev.h"
#include "mm/heap.h"
#include "io/fs/vfs.h"
#include "io/dev/rp.h"
#include "utils.h"
#include "ddk/fs.h"
#include "logging.h"
#include "rtl/string.h"
#include "rtl/order.h"

struct FatGetEntryContext
{
    struct IoRp *rp;
    struct FatVolume *vol;
    struct FatDirectory *list;
    union FsRequest *req;
    uint32_t cluster;
    uint32_t size;
    char lastFileName[512];
};

static void FatFillVfsNode(struct IoVfsNode *n, struct FatDirectory *e, struct FatVolume *vol)
{
    //store cluster for file data start
    n->ref[1].u32 = ((uint32_t)RtlLeU16(e->fstClusLo) | ((uint32_t)RtlLeU16(e->fstClusHi) << 16));
    n->device = IoGetDeviceStackTop(vol->vol);
    n->fsType = IO_VFS_FS_PHYSICAL;
    n->type = (e->attributes & FAT_ATTR_DIRECTORY) ? IO_VFS_DIRECTORY : IO_VFS_FILE;
    n->flags = IO_VFS_FLAG_NO_HARD_LINKS;
    n->references.links = 1;
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
                            status = BAD_PARAMETER;
                    }
                    else //8.3 file name
                    {
                        if(0 != FatDosNameToFileName(name, ctx->list[i].name))
                            status = BAD_PARAMETER;
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
                ctx->rp->status = NOT_FOUND;
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
        ctx->rp->status = NOT_FOUND;
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

    union FsRequest *req = rp->payload.deviceControl.data;
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