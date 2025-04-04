#include "vga.h"
#include "io/dev/rp.h"
#include "io/dev/dev.h"
#include "io/video/output.h"
#include "mm/heap.h"
#include "vesa.h"
#include "hal/video.h"
#include "rtl/string.h"
#include "rtl/stdio.h"
#include "ddk/display.h"

#define DISPLAY_DEVICE_ID_PREFIX "DISPLAY"

void VgaEnumerateDisplays(struct IoRp *rp)
{
    STATUS status = OK;
    struct VgaDisplayInfo *info = NULL;
    struct VgaAdapterInfo *adapter = IoGetCurrentRpPosition(rp)->privateData;
    struct IoDeviceObject *dev = NULL;

    if(VGA_INFO_ADAPTER != adapter->type)
    {
        rp->status = DEVICE_NOT_AVAILABLE;
        IoFinalizeRp(rp);
        return;
    }

    info = MmAllocateKernelHeapZeroed(sizeof(*info));
    if(NULL == info)
    {
        rp->status = OUT_OF_RESOURCES;
        IoFinalizeRp(rp);
        return;
    }

    status = VgaVesaGetDisplayInfo(info);
    if(OK != status)
    {
        rp->status = status;
        MmFreeKernelHeap(info);
        IoFinalizeRp(rp);
        return;
    }

    status = IoCreateDevice(rp->device->driverObject, IO_DEVICE_TYPE_VIDEO, 0, &dev);
    if(OK != status)
    {
        rp->status = status;
        MmFreeKernelHeap(info);
        IoFinalizeRp(rp);
        return;
    }

    info->type = VGA_INFO_DISPLAY;
    info->adapter = IoGetCurrentRpPosition(rp);
    dev->privateData = info;

    status = IoRegisterDevice(dev, IoGetCurrentRpPosition(rp));
    if(OK != status)
    {
        rp->status = status;
        MmFreeKernelHeap(info);
        IoDestroyDevice(dev);
        IoFinalizeRp(rp);
        return;
    }

    size_t mode = VgaFindBestPerfectMatchingMode(info, adapter);
    if((size_t)(-1) == mode)
    {
        //no matching video mode, use standard 640x480@60Hz that should be always supported
        for(mode = 0; mode < adapter->modeCount; mode++)
        {
            if((640 == adapter->mode[mode].config.width) && (480 == adapter->mode[mode].config.height))
                break;
        }
        if(mode >= adapter->modeCount)
            mode = (size_t)(-1);
    }

    if((size_t)(-1) == mode)
    {
        //absolutely zero matching modes? then fail
        rp->status = DEVICE_NOT_AVAILABLE;
        MmFreeKernelHeap(info);
        IoDestroyDevice(dev);
        IoFinalizeRp(rp);
        return;
    }

    HalVideoDeinit();

    status = VgaVesaSetMode(adapter->mode[mode].mode, VgaFindTimingForMode(info, adapter, mode));
    if(OK != status)
    {
        rp->status = status;
        IoDestroyDevice(dev);
        IoFinalizeRp(rp);
        return;
    }

    status = IoRegisterFrameBuffer(dev, &adapter->mode[mode].config, adapter->mode[mode].lfb);
    if(OK != status)
    {
        rp->status = status;
        IoDestroyDevice(dev);
        IoFinalizeRp(rp);
        return;
    }

    rp->status = OK;
    IoFinalizeRp(rp);
}


const struct VgaTiming* VgaFindTimingForMode(const struct VgaDisplayInfo *display, const struct VgaAdapterInfo *adapter, size_t mode)
{
    if(mode >= adapter->modeCount)
        return false;

    const struct VgaTiming *timing = display->timing;

    for(size_t i = 0; i < display->timingCount; i++)
    {
        if(timing[i].usable.x != adapter->mode[mode].config.width)
            continue;
        if(timing[i].usable.y != adapter->mode[mode].config.height)
            continue;
        if(timing[i].flags.doubleScan && !adapter->mode[mode].available.doubleScan)
            continue;
        if(timing[i].flags.interlaced && !adapter->mode[mode].available.interlaced)
            continue;    
        if((0 != adapter->mode[mode].maxClock) && (timing[i].clock > adapter->mode[mode].maxClock))
            continue;
        
        return &display->timing[i];
    }

    return NULL;    
}

size_t VgaFindModeForTiming(const struct VgaDisplayInfo *display, const struct VgaAdapterInfo *adapter, size_t timing)
{
    if(timing >= display->timingCount)
        return (size_t)(-1);

    const struct VgaTiming *t = &display->timing[timing];

    for(size_t i = 0; i < adapter->modeCount; i++)
    {
        if(t->usable.x != adapter->mode[i].config.width)
            continue;
        if(t->usable.y != adapter->mode[i].config.height)
            continue;
        if(t->flags.doubleScan && !adapter->mode[i].available.doubleScan)
            continue;
        if(t->flags.interlaced && !adapter->mode[i].available.interlaced)
            continue;    
        if((0 != adapter->mode[i].maxClock) && (t->clock > adapter->mode[i].maxClock))
            continue;
        
        return i;
    }

    return (size_t)(-1);      
}

bool VgaCheckModeCompatibility(const struct VgaDisplayInfo *display, const struct VgaAdapterInfo *adapter, size_t mode)
{
    return (NULL != VgaFindTimingForMode(display, adapter, mode));
}

bool VgaCheckModeCompatibilityByTiming(const struct VgaDisplayInfo *display, const struct VgaAdapterInfo *adapter, size_t timing)
{
    return ((size_t)(-1) != VgaFindModeForTiming(display, adapter, timing));
}

size_t VgaFindBestPerfectMatchingMode(const struct VgaDisplayInfo *display, const struct VgaAdapterInfo *adapter)
{
    size_t best = (size_t)(-1);

    if((size_t)(-1) != display->preferredTiming)
    {
        best = VgaFindModeForTiming(display, adapter, display->preferredTiming);
        if((size_t)(-1) != best)
        {
            return best;
        }
    }

    for(size_t i = 0; i < adapter->modeCount; i++)
    {
        if(!VgaCheckModeCompatibility(display, adapter, i))
            continue;
        
        if((size_t)(-1) == best)
        {
            best = i;
        }
        else
        {
            if((adapter->mode[best].config.width > adapter->mode[i].config.width)
                || (adapter->mode[best].config.height > adapter->mode[i].config.height))
            {
                best = i;
            }
        }
    }

    return best;
}

void VgaGetDisplayDeviceId(struct IoRp *rp)
{
    struct VgaDisplayInfo *info = IoGetCurrentRpPosition(rp)->privateData;
    char *deviceId = NULL, **compatibleIds = NULL;

    if(VGA_INFO_DISPLAY != info->type)
    {
        rp->status = RP_PROCESSING_FAILED;
        IoFinalizeRp(rp);
        return;
    }   

    deviceId = MmAllocateKernelHeap(64);
    if(NULL == deviceId)
    {
        rp->status = OUT_OF_RESOURCES;
        IoFinalizeRp(rp);
        return;
    }

    compatibleIds = RtlAllocateStringTable(IO_MAX_COMPATIBLE_DEVICE_IDS, 1, 64);
    if(NULL == compatibleIds)
    {
        MmFreeKernelHeap(deviceId);
        rp->status = OUT_OF_RESOURCES;
        IoFinalizeRp(rp);
        return;
    }

    snprintf(deviceId, 64, DISPLAY_DEVICE_ID_PREFIX "/%s", info->id);
    strcpy(compatibleIds[0], DDK_DISPLAY_GENERIC_COLOR_FB_DEVICE_ID);      

    rp->payload.deviceId.mainId = deviceId;
    rp->payload.deviceId.compatibleId = compatibleIds;

    rp->status = OK;
    IoFinalizeRp(rp);
}
