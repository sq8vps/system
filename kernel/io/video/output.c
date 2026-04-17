#include "output.h"
#include "io/dev/dev.h"
#include "ke/core/mutex.h"
#include "mm/mmio.h"
#include "mm/heap.h"
#include "hal/interrupt.h"
#include "io/log/syslog.h"

/**
 * @brief Maximum number of registered video outputs
 */
#define IO_MAX_VIDEO_OUTPUTS 16

/**
 * @brief Maximum number of video event handlers per video output
 */
#define IO_MAX_VIDEO_EVENT_HANDLERS 16

struct IoFrameBufferHandle
{
    struct IoFrameBufferConfig config; /**< Frame buffer configuration */
    PADDRESS address; /**< Physical frame buffer address */
    void *pfb; /**< Mapped physical frame buffer pointer */
    void *fb; /**< Frame buffer in main memory */
    void *ufb; /**< User frame buffer */
    size_t size; /**< Mapped page-aligned frame buffer size */
    size_t pSize; /**< Physical frame buffer size */
};

static struct
{
    struct
    {
        bool used; /**< Is this slot occupied? */
        const struct IoDeviceObject *dev; /**< Associated device */
        KeSpinlock lock; /**< Handle lock */
        enum IoVideoType type; /**< Video output type */
        union
        {
            struct IoFrameBufferHandle fb;
        };
        struct
        {
            bool used; /**< Is this slot occupied? */
            IoVideoConfigChangeHandler handler; /**< Video config change callback */
            void *context; /**< Video config change callback context */
        } changeHandler[IO_MAX_VIDEO_EVENT_HANDLERS];
        
    } list[IO_MAX_VIDEO_OUTPUTS];
    KeSpinlock lock;
} IoVideoOutputs = {.lock = KeSpinlockInitializer, .list[0 ... IO_MAX_VIDEO_OUTPUTS - 1] = {.used = false, .changeHandler[0 ... IO_MAX_VIDEO_EVENT_HANDLERS - 1] = {.used = false}}};

STATUS IoRegisterFrameBuffer(const struct IoDeviceObject *dev, const struct IoFrameBufferConfig *config, PADDRESS address)
{
    STATUS status = OK;

    size_t pSize = config->pitch * config->height;
    size_t fbSize = ALIGN_UP(pSize, PAGE_SIZE);

    if((pSize & 0xF) || (address & 0xF))
        return BAD_ALIGNMENT;
    
    int handle = -1;
    PRIO prio = KeAcquireDpcLevelSpinlock(&(IoVideoOutputs.lock));
    for(size_t i = 0; i < IO_MAX_VIDEO_OUTPUTS; i++)
    {
        if(IoVideoOutputs.list[i].used && (dev == IoVideoOutputs.list[i].dev))
        {
            handle = i;
            break;
        }
    }
    KeReleaseSpinlock(&(IoVideoOutputs.lock), prio);
    
    void *pfb = MmMapMmIo(address, pSize);
    if(NULL == pfb)
        return OUT_OF_RESOURCES;
    
    void *fb = MmAllocateKernelHeapAligned(fbSize, 16);
    if(NULL == fb)
    {
        MmUnmapMmIo(pfb);
        return OUT_OF_RESOURCES;
    }

    void *ufb = MmAllocateKernelHeapAligned(fbSize, 16);
    if(NULL == ufb)
    {
        MmFreeKernelHeap(fb);
        MmUnmapMmIo(pfb);
        return OUT_OF_RESOURCES;
    }

    if(-1 != handle) //updating frame buffer config
    {
        void *oldPfb, *oldFb;
        prio = KeAcquireDpcLevelSpinlock(&(IoVideoOutputs.list[handle].lock));
        oldPfb = IoVideoOutputs.list[handle].fb.pfb;
        oldFb = IoVideoOutputs.list[handle].fb.fb;
        IoVideoOutputs.list[handle].fb.address = address;
        IoVideoOutputs.list[handle].fb.fb = fb;
        IoVideoOutputs.list[handle].fb.pfb = pfb;
        IoVideoOutputs.list[handle].fb.ufb = ufb;
        IoVideoOutputs.list[handle].fb.size = fbSize;
        IoVideoOutputs.list[handle].fb.pSize = pSize;
        IoVideoOutputs.list[handle].fb.config = *config;

        union IoVideoOutput c;
        c.fb.config = *config;
        c.fb.fb = ufb;
        for(size_t i = 0; i < IO_MAX_VIDEO_EVENT_HANDLERS; i++)
        {
            if(IoVideoOutputs.list[handle].changeHandler[i].used)
                IoVideoOutputs.list[handle].changeHandler[i].handler(handle, &c, IoVideoOutputs.list[handle].changeHandler[i].context);
        }

        KeReleaseSpinlock(&(IoVideoOutputs.list[handle].lock), prio);
        MmFreeKernelHeap(oldFb);
        MmUnmapMmIo(oldPfb);
    }
    else //new frame buffer
    {
        status = OUT_OF_RESOURCES;
        prio = KeAcquireDpcLevelSpinlock(&(IoVideoOutputs.lock));
        for(size_t i = 0; i < IO_MAX_VIDEO_OUTPUTS; i++)
        {
            if(!IoVideoOutputs.list[i].used)
            {
                IoVideoOutputs.list[i].dev = dev;
                IoVideoOutputs.list[i].fb.address = address;
                IoVideoOutputs.list[i].fb.config = *config;
                IoVideoOutputs.list[i].fb.fb = fb;
                IoVideoOutputs.list[i].fb.pfb = pfb;
                IoVideoOutputs.list[i].fb.ufb = ufb;
                IoVideoOutputs.list[i].fb.size = fbSize;
                IoVideoOutputs.list[i].fb.pSize = pSize;
                IoVideoOutputs.list[i].type = IO_VIDEO_FRAME_BUFFER;
                IoVideoOutputs.list[i].used = true;
                status = OK;
                handle = i;
                break;
            }
        }
        KeReleaseSpinlock(&(IoVideoOutputs.lock), prio);
        if(OK != status)
        {
            MmFreeKernelHeap(fb);
            MmFreeKernelHeap(ufb);
            MmUnmapMmIo(pfb);
        }
        else
        {
            union IoVideoOutput c;
            c.fb.config = *config;
            c.fb.fb = ufb;
            prio = KeAcquireDpcLevelSpinlock(&(IoVideoOutputs.list[handle].lock));
            for(size_t i = 0; i < IO_MAX_VIDEO_EVENT_HANDLERS; i++)
            {
                if(IoVideoOutputs.list[handle].changeHandler[i].used)
                    IoVideoOutputs.list[handle].changeHandler[i].handler(handle, &c, IoVideoOutputs.list[handle].changeHandler[i].context);
            }
            KeReleaseSpinlock(&(IoVideoOutputs.list[handle].lock), prio);
        }
    }
    
    return status;
}

STATUS IoGetVideoOutput(int handle, IoVideoConfigChangeHandler changeHandler, void *context, union IoVideoOutput *const output, enum IoVideoType *const type)
{
    STATUS status = OK;

    if((handle < 0) || (handle >= IO_MAX_VIDEO_OUTPUTS))
        return NOT_FOUND;
    
    PRIO prio = KeAcquireDpcLevelSpinlock(&(IoVideoOutputs.lock));
    if(IoVideoOutputs.list[handle].used)
    {
        *type = IoVideoOutputs.list[handle].type;
        switch(IoVideoOutputs.list[handle].type)
        {
            case IO_VIDEO_FRAME_BUFFER:
                output->fb.config = IoVideoOutputs.list[handle].fb.config;
                output->fb.fb = IoVideoOutputs.list[handle].fb.ufb;
                break;
            default:
                break;
        }
    }
    else
        status = NOT_FOUND;

    if(OK == status)
    {
        PRIO prio = KeAcquireDpcLevelSpinlock(&(IoVideoOutputs.list[handle].lock));
        status = OUT_OF_RESOURCES;
        for(size_t i = 0; i < IO_MAX_VIDEO_EVENT_HANDLERS; i++)
        {
            if(!IoVideoOutputs.list[handle].changeHandler[i].used)
            {
                IoVideoOutputs.list[handle].changeHandler[i].used = true;
                IoVideoOutputs.list[handle].changeHandler[i].handler = changeHandler;
                IoVideoOutputs.list[handle].changeHandler[i].context = context;
                status = OK;
                break;
            }
        }
        KeReleaseSpinlock(&(IoVideoOutputs.list[handle].lock), prio);
    }
    KeReleaseSpinlock(&(IoVideoOutputs.lock), prio);
    return status;
}

STATUS IoRemoveVideoOutputConfigChangeHandler(int handle, IoVideoConfigChangeHandler changeHandler, void *context)
{
    STATUS status = OK;

    if((handle < 0) || (handle >= IO_MAX_VIDEO_OUTPUTS))
        return NOT_FOUND;
    
    PRIO prio = KeAcquireDpcLevelSpinlock(&(IoVideoOutputs.lock));
    if(!IoVideoOutputs.list[handle].used)
    {
        status = NOT_FOUND;  
    }
    else
    {
        PRIO prio = KeAcquireDpcLevelSpinlock(&(IoVideoOutputs.list[handle].lock));
        status = NOT_FOUND;
        for(size_t i = 0; i < IO_MAX_VIDEO_EVENT_HANDLERS; i++)
        {
            if(IoVideoOutputs.list[handle].changeHandler[i].used
                && (changeHandler == IoVideoOutputs.list[handle].changeHandler[i].handler)
                && (context == IoVideoOutputs.list[handle].changeHandler[i].context))
            {
                IoVideoOutputs.list[handle].changeHandler[i].used = false;
                IoVideoOutputs.list[handle].changeHandler[i].handler = NULL;
                IoVideoOutputs.list[handle].changeHandler[i].context = NULL;
                status = OK;
                break;
            }
        }
        KeReleaseSpinlock(&(IoVideoOutputs.list[handle].lock), prio);
    }
    KeReleaseSpinlock(&(IoVideoOutputs.lock), prio);
    return status;
}

#include "assert.h"

void IoDrawVideo(int handle)
{
    if(unlikely((handle < 0) || (handle >= IO_MAX_VIDEO_OUTPUTS)))
        return;
    
    PRIO prio = KeAcquireDpcLevelSpinlock(&(IoVideoOutputs.list[handle].lock));
    if(unlikely(!IoVideoOutputs.list[handle].used))
    {
        KeReleaseSpinlock(&(IoVideoOutputs.list[handle].lock), prio);
        return;
    }

    if(IO_VIDEO_FRAME_BUFFER == IoVideoOutputs.list[handle].type)
    {
        volatile uint32_t *ufb = IoVideoOutputs.list[handle].fb.ufb;
        uint32_t *fb = IoVideoOutputs.list[handle].fb.fb;
        uint32_t *pfb = IoVideoOutputs.list[handle].fb.pfb;
        size_t size = (IoVideoOutputs.list[handle].fb.size < IoVideoOutputs.list[handle].fb.pSize) ? IoVideoOutputs.list[handle].fb.size : IoVideoOutputs.list[handle].fb.pSize;
        for(size_t i = 0; i < size / 4; i++)
        {
            if(ufb[i] != fb[i])
            {
                fb[i] = ufb[i];
                pfb[i] = ufb[i];
            }
        }
    }

    KeReleaseSpinlock(&(IoVideoOutputs.list[handle].lock), prio);
}