#include "fsctrl.h"
#include "io/dev/rp.h"
#include "io/dev/dev.h"
#include "ddk/fs.h"
#include "structs.h"
#include "fat.h"
#include "io/fs/vfs.h"
#include "io/dev/op.h"
#include "mm/heap.h"

STATUS FatFsControl(struct IoRp *rp)
{
    if(IO_RP_FILESYSTEM_CONTROL != rp->code)
        return RP_PROCESSING_FAILED;

    struct FatVolume *vol = rp->device->privateData;
    if(NULL == vol)
        return RP_PROCESSING_FAILED;

    return FatGetNode(rp, vol);
}

struct FatUpdateFileAttributesCallbackContext
{
    struct FatVolume *vol;
    struct IoVfsNode *node;
    void *buffer;
    bool write;
};

static void FatUpdateFileAttributesCallback(STATUS status, size_t actualSize, void *context)
{
    struct FatUpdateFileAttributesCallbackContext *ctx = context;
    if(OK == status)
    {
        if(!ctx->write)
        {
            struct FatDirectory *e = (struct FatDirectory*)((uintptr_t)ctx->buffer + (uintptr_t)(ctx->node->ref[0].u64 % ctx->vol->disk->blockSize));
            ctx->write = true;
            //TODO: update timestamps
            e->fileSize = ctx->node->size;
            status = IoReadWrite(true, ctx->vol->disk, NULL, (ctx->node->ref[0].u64 / ctx->vol->disk->blockSize) * ctx->vol->disk->blockSize, 
                ctx->vol->disk->blockSize, ctx->buffer, FatUpdateFileAttributesCallback, ctx, false);
        }
        else
        {
            MmFreeKernelHeap(ctx->buffer);
            MmFreeKernelHeap(ctx);
            status = OK;
        }
    }

    if(OK != status)
    {
        MmFreeKernelHeap(ctx->buffer);
        MmFreeKernelHeap(ctx);
    }
}

STATUS FatUpdateFileAttributes(struct FatVolume *vol, struct IoVfsNode *node)
{
    STATUS status = OK;

    void *buffer = MmAllocateKernelHeapAligned(vol->disk->blockSize, IO_DEV_REQUIRED_ALIGNMENT(vol->disk));
    if(NULL == buffer)
        return OUT_OF_RESOURCES;
    
    struct FatUpdateFileAttributesCallbackContext *ctx = MmAllocateKernelHeap(sizeof(*ctx));
    if(NULL == ctx)
    {
        MmFreeKernelHeap(buffer);
        return OUT_OF_RESOURCES;
    }
    
    ctx->buffer = buffer;
    ctx->node = node;
    ctx->vol = vol;
    ctx->write = false;

    status = IoReadWrite(false, vol->disk, NULL, (node->ref[0].u64 / vol->disk->blockSize) * vol->disk->blockSize, 
        vol->disk->blockSize, buffer, FatUpdateFileAttributesCallback, ctx, false);

    if(OK != status)
    {
        MmFreeKernelHeap(buffer);
        MmFreeKernelHeap(ctx);
    }
    return status;
}