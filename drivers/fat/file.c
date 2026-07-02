#include "fat.h"
#include "structs.h"
#include "io/dev/rp.h"
#include "io/fs/vfs.h"
#include "io/dev/dev.h"
#include "ddk/fs.h"
#include "logging.h"
#include "rtl/string.h"
#include "utils.h"
#include "mm/heap.h"

struct FatCreateContext
{
    struct IoRp *rp;
    struct FatVolume *vol;
    struct FatDirectory *list;
    union FsRequest *req;
    struct FatDirectory *entries;
    size_t cluster;
    size_t nextEntry;
    size_t entryCount;
    size_t size;
    bool isRootAndFat16;
    bool writeStep;
    uint32_t targetCluster;
};

static STATUS FatCreateDirectoryEntry(struct FatDirectory **entries, size_t *entryCount, const union FsRequest *req)
{
    STATUS status = OK;
    *entries = nullptr;
    struct FatDirectory *entry = nullptr;
    char *ucs2Name = MmAllocateKernelHeap(2 * (RtlStrlen(req->create.name) + 1));
    size_t ucs2Size = 0;
    size_t lfnEntryCount = 0;
    if(nullptr == ucs2Name)
    {
        status = OUT_OF_RESOURCES;
        goto leave;
    }
    
    ucs2Size = FatUtf8ToUcs2(ucs2Name, req->create.name);
    if((size_t)(-1) == ucs2Size)
    {
        status = BAD_PARAMETER;
        goto leave;
    }

    if(0 != (ucs2Size % 13))
    {
        ucs2Name[ucs2Size * 2] = '\0';
        ucs2Name[ucs2Size * 2 + 1] = '\0';
        ++ucs2Size;
        lfnEntryCount += (ucs2Size / 13);
        if(ucs2Size % 13)
            ++lfnEntryCount;
    }
    else
    {
        lfnEntryCount += (ucs2Size / 13);
    }

    if(lfnEntryCount > FAT_MAX_LONG_FILE_NAME_ENTRIES)
    {
        status = BAD_PARAMETER;
        goto leave;
    }

    *entryCount = lfnEntryCount + 1;
    //allocate N entries for LFN, 1 for the main entry, and 1 empty entry that might be needed at the end
    *entries = MmAllocateKernelHeapZeroed((lfnEntryCount + 2) * sizeof(**entries));
    if(nullptr == *entries)
    {
        status = OUT_OF_RESOURCES;
        goto leave;
    }
    entry = *entries;

    //insert directory entry first

    switch(req->create.type)
    {
        case IO_VFS_DIRECTORY:
            entry[lfnEntryCount].attributes = FAT_ATTR_DIRECTORY;
            break;
        case IO_VFS_FILE:
            break;
        default:
            return BAD_PARAMETER;
    }
    if(req->create.flags & IO_VFS_FLAG_READ_ONLY)
        entry[lfnEntryCount].attributes |= FAT_ATTR_READ_ONLY;
    if(req->create.flags & IO_VFS_FLAG_HIDDEN)
        entry[lfnEntryCount].attributes |= FAT_ATTR_HIDDEN;
    
    //TODO: timestamps
    FatHashFileName(req->create.name, entry[lfnEntryCount].name);

    uint8_t checksum = FatDosNameChecksum(entry[lfnEntryCount].name);

    //then all the LFN entries
    ucs2Size *= 2;
    size_t ucs2Remaining = ucs2Size;
    struct FatLongFileNameEntry *lfn = (struct FatLongFileNameEntry*)&entry[lfnEntryCount - 1];
    for(size_t i = 0; i < lfnEntryCount; i++)
    {
        bool lastEntry = !!(i == (lfnEntryCount - 1));
        if(lastEntry)
            RtlMemset(lfn, 0xFF, sizeof(*lfn)); //name needs to be padded with 0xFF
        lfn->ord = i + 1;
        if(lastEntry)
            lfn->ord |= FAT_DIRECTORY_ENTRY_LAST_LONG_NAME_ENTRY;
        lfn->attributes = FAT_ATTR_LONG_NAME;
        lfn->type = 0;
        lfn->fstClusLo = 0;
        lfn->checksum = checksum;
        if(!lastEntry)
        {
            RtlMemcpy(lfn->name1, &ucs2Name[2 * i * 13], 10);
            RtlMemcpy(lfn->name2, &ucs2Name[2 * i * 13 + 10], 12);
            RtlMemcpy(lfn->name3, &ucs2Name[2 * i * 13 + 22], 4);
            ucs2Remaining -= 26;
        }
        else //last entry
        {
            RtlMemcpy(lfn->name1, &ucs2Name[2 * i * 13], (ucs2Remaining > 10) ? 10 : ucs2Remaining);
            if(ucs2Remaining > 10)
                RtlMemcpy(lfn->name2, &ucs2Name[2 * i * 13 + 10], (ucs2Remaining > 22) ? 12 : (ucs2Remaining - 10));
            if(ucs2Remaining > 22)
                RtlMemcpy(lfn->name3, &ucs2Name[2 * i * 13 + 22], ucs2Remaining - 22);
        }
        --lfn;
    }
    
leave:
    if(OK != status)
    {
        MmFreeKernelHeap(*entries);
    }
    MmFreeKernelHeap(ucs2Name);
    return status;
}

static void FatCreateCallback(STATUS status, size_t actualSize, void *context)
{
    struct FatCreateContext *ctx = context;
    uint64_t offset = 0;

    if(ctx->size != actualSize)
    {
        ctx->rp->status = OPERATION_INCOMPLETE;
        goto leave;
    }

    if(OK == status)
    {
        if(!ctx->writeStep)
        {        
            size_t available = 0;
            size_t first = 0;
            size_t i = 0;
            while(((uintptr_t)&(ctx->list[i]) - (uintptr_t)&(ctx->list[0])) < ctx->size)
            {
                if(FAT_DIRECTORY_ENTRY_EMPTY_LAST == ctx->list[i].name[0])
                {
                    first = i;
                    available = (ctx->vol->bytesPerCluster / sizeof(*ctx->list)) - i;
                    break;
                }
                i++;
            }
            
            status = FatCreateDirectoryEntry(&ctx->entries, &ctx->entryCount, ctx->req);
            if(OK != status)
            {
                ctx->rp->status = status;
                goto leave;
            }

            if(available < ctx->entryCount)
            {
                //in FAT16/12, the number of entries is constant
                if(ctx->isRootAndFat16)
                {
                    ctx->rp->status = OUT_OF_RESOURCES;
                    goto leave;
                }
                else //in FAT32, assign new clusters
                {
                    size_t clusters = (ctx->entryCount - available) / sizeof(struct FatDirectory);
                    if((ctx->entryCount - available) % sizeof(struct FatDirectory))
                        ++clusters;

                    if(0 == FatReserveClusters(ctx->vol, ctx->cluster, clusters))
                    {
                        FatFreeClusters(ctx->vol, ctx->cluster);
                        ctx->rp->status = OUT_OF_RESOURCES;
                        goto leave;
                    }
                    if(0 == available)
                        ctx->cluster = FatGetNextCluster(ctx->vol, ctx->cluster);
                }
            }
    
            offset = FAT_GET_OFFSET(ctx->vol, ctx->cluster);

            if(IO_VFS_DIRECTORY == ctx->req->create.type)
            {
                ctx->targetCluster = FatReserveClusters(ctx->vol, 0, 1);
                if(0 == ctx->targetCluster)
                {
                    ctx->rp->status = OUT_OF_RESOURCES;
                    goto leave;
                }
            }

            ctx->entries[ctx->entryCount - 1].fstClusLo = ctx->targetCluster & 0xFFFF;
            ctx->entries[ctx->entryCount - 1].fstClusHi = ctx->targetCluster >> 16;

            if(0 == available)
                ctx->nextEntry = MIN(ctx->entryCount, (ctx->vol->bytesPerCluster / sizeof(*ctx->entries)));
            else
                ctx->nextEntry = MIN(ctx->entryCount, available);

            if(ctx->entryCount < available)
                ++ctx->nextEntry; //write empty entry

            RtlMemcpy(&ctx->list[first], ctx->entries, ctx->nextEntry * sizeof(*ctx->entries));

            ctx->writeStep = true;

            status = IoReadWrite(true, ctx->vol->disk, nullptr, offset, ctx->size, ctx->list, FatCreateCallback, ctx, false);
            if(OK == status)
                return;
            ctx->rp->status = status;
            goto leave;
        }
        else //ctx->writeStep == true
        {
            if(ctx->nextEntry >= ctx->entryCount)
            {
                if(0 != ctx->targetCluster)
                {
                    RtlMemset(ctx->list, 0, ctx->size);
                    offset = FAT_GET_OFFSET(ctx->vol, ctx->targetCluster);
                    ctx->targetCluster = 0;
                    status = IoReadWrite(true, ctx->vol->disk, nullptr, offset, ctx->size, ctx->list, FatCreateCallback, ctx, false);
                    if(OK == status)
                        return;
                    ctx->rp->status = status;
                    goto leave;
                }
                ctx->rp->status = status;
                FatUpdateOnDisk(ctx->vol);
                goto leave;
            }
            else
            {
                size_t count = MIN((ctx->entryCount - ctx->nextEntry), (ctx->vol->bytesPerCluster / sizeof(*ctx->entries)));
                size_t thisEntry = ctx->nextEntry;
                ctx->nextEntry += count;
                if(ctx->nextEntry >= ctx->entryCount)
                    ++count; //write empty entry

                RtlMemcpy(ctx->list, &ctx->entries[thisEntry], count * sizeof(*ctx->entries));
                ctx->cluster = FatGetNextCluster(ctx->vol, ctx->cluster);
                offset = FAT_GET_OFFSET(ctx->vol, ctx->cluster);
                status = IoReadWrite(true, ctx->vol->disk, nullptr, offset, ctx->size, 
                    &ctx->entries[thisEntry], FatCreateCallback, ctx, false);
                if(OK == status)
                    return;
                ctx->rp->status = status;
                goto leave;
            }
        }
    }
    else
    {
        ctx->rp->status = status;
        goto leave;
    }

leave:
    IoFinalizeRp(ctx->rp);
    MmFreeKernelHeap(ctx->entries);
    MmFreeKernelHeap(ctx->list);
    MmFreeKernelHeap(ctx);
}

STATUS FatCreateFile(struct IoRp *rp, struct FatVolume *vol)
{
    union FsRequest *req = rp->payload.deviceControl.data;;
    if((nullptr == req) || (nullptr == req->create.node) || (nullptr == req->create.name))
        return BAD_PARAMETER;

    uint32_t cluster = 0;
    uint64_t offset = 0;
    uint64_t size = 0;
    bool isRootAndFat16 = false; //parent directory is root and it's FAT16 or FAT12

    if(IO_VFS_MOUNT_POINT != req->create.parent->type)
        cluster = req->create.parent->ref[1].u32; //if parent is not the mountpoint, we may obtain the target parent cluster
    else if(FAT32 == vol->type)
        cluster = vol->rootCluster; //if parent is a mountpoint but this is FAT32, we make use of clusters
    else
        isRootAndFat16 = true; //if parent is a mountpoint but this is not FAT32, the root directory is contiguous

    if(!isRootAndFat16)
        cluster = FatGetLastCluster(vol, cluster);
    
    if(!isRootAndFat16)
    {
        size = vol->sectorsPerCluster * vol->disk->blockSize;
        offset = FAT_GET_OFFSET(vol, cluster);
    }
    else
    {
        size = vol->rootEntryCount * 32;
        offset = FAT_ROOT_OFFSET(vol);
    }

    struct FatDirectory *list = MmAllocateKernelHeapAligned(size, IO_DEV_REQUIRED_ALIGNMENT(vol->disk));
    if(NULL == list)
        return OUT_OF_RESOURCES;

    struct FatCreateContext *ctx = MmAllocateKernelHeap(sizeof(*ctx));
    if(NULL == ctx)
    {
        MmFreeKernelHeap(list);
        return OUT_OF_RESOURCES;
    }

    ctx->rp = rp;
    ctx->vol = vol;
    ctx->size = size;
    ctx->list = list;
    ctx->req = req;
    ctx->cluster = cluster;
    ctx->isRootAndFat16 = isRootAndFat16;
    ctx->writeStep = false;
    IoMarkRpPending(rp);

    return IoReadWrite(false, vol->disk, NULL, offset, size, list, FatCreateCallback, ctx, false);
}