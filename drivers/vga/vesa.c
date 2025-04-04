#include "vesa.h"
#include "vga.h"
#include "mm/heap.h"
#include "hal/i686/emu/reg.h"
#include "hal/i686/emu/emu.h"
#include "rtl/string.h"
#include "rtl/order.h"
#include "rtl/stdio.h"
#include "timings.h"

struct VbeInfoBlock
{
    char signature[4];
    uint16_t version;
    uint32_t oemString;
    uint8_t capabilities[4];
    uint32_t modeList;
    uint16_t memory;
    uint16_t revision;
    uint32_t vendorName;
    uint32_t productName;
    uint32_t productRev;
    uint8_t reserved[222];
    uint8_t data[256];
} PACKED;

struct VbeModeInfoBlock
{
    uint16_t attributes;
    uint8_t uninteresting[14];
    uint16_t bytesPerScanLine;
    uint16_t xRes;
    uint16_t yRes;
    uint8_t xCharSize;
    uint8_t yCharSize;
    uint8_t numberOfPlanes;
    uint8_t bitsPerPixel;
    uint8_t numberOfBanks;
    uint8_t memoryModel;
    uint8_t bankSize;
    uint8_t numberOfPages;
    uint8_t reserved;

    uint8_t redMaskSize;
    uint8_t redPosition;
    uint8_t greenMaskSize;
    uint8_t greenPosition;
    uint8_t blueMaskSize;
    uint8_t bluePosition;
    uint8_t rsvdMaskSize;
    uint8_t rsvdPosition;
    uint8_t directColorInfo;

    uint32_t lfbBase;
    uint8_t reserved2[6];

    uint16_t linBytesPerScanLine;
    uint8_t bnkNumberOfPages;
    uint8_t linNumberOfPages;
    uint8_t linRedMaskSize;
    uint8_t linRedPosition;
    uint8_t linGreenMaskSize;
    uint8_t linGreenPosition;
    uint8_t linBlueMaskSize;
    uint8_t linBluePosition;
    uint8_t linRsvdMaskSize;
    uint8_t linRsvdPosition;
    uint32_t maxPixelClock;

    uint8_t reserved3[189];
} PACKED;

enum
{
    VBE_MEMORY_MODEL_DIRECT_COLOR = 0x06,
};


struct VbeEdid
{
    uint64_t header;
    uint16_t manufacturer;
    uint16_t product;
    uint32_t serial;
    uint8_t week;
    uint8_t year;
    
    uint8_t version;
    uint8_t revision;
    
    uint8_t inputDefinition;
    uint8_t xSize;
    uint8_t ySize;
    uint8_t gamma;
    uint8_t features;

    struct
    {
        uint8_t data[10];
    } color;

    uint8_t establishedTimings[3];

    struct
    {
        uint8_t xRes;
        uint8_t arRefresh;
    } standardTimings[8];

    union
    {
        struct
        {
            uint16_t pixelClock;
            uint8_t xResLo;
            uint8_t xBlankLo;
            uint8_t xResBlankHi;

            uint8_t yResLo;
            uint8_t yBlankLo;
            uint8_t yResBlankHi;

            uint8_t xPorchLo;
            uint8_t xSyncLo;
            uint8_t yPorchSyncLo;
            uint8_t xyPorchSyncHi;

            uint8_t xSizeLo;
            uint8_t ySizeLo;
            uint8_t xySizeHi;

            uint8_t xBorder;
            uint8_t yBorder;

            uint8_t flags;
        } detailedTiming;

        struct
        {
            uint16_t header;
            uint8_t reserved;
            uint8_t tag;
            uint8_t flags;
            uint8_t minVRate;
            uint8_t maxVRate;
            uint8_t minHRate;
            uint8_t maxHRate;
            uint8_t maxClock;
            uint8_t timingSupportFlags;
            uint8_t timingData[7];
        } limits;
        
    } descriptor[4];
    
    uint8_t extensionBlocks;
    uint8_t checksum;
} PACKED;

struct VbeCrtc
{
    uint16_t xTotal;
    uint16_t xSyncStart;
    uint16_t xSyncEnd;
    uint16_t yTotal;
    uint16_t ySyncStart;
    uint16_t ySyncEnd;
    uint8_t flags;
    uint32_t clock;
    uint16_t refresh;
    uint8_t reserved[40];
} PACKED;

#define VBE_INT 0x10

enum
{
    VBE_AH_DO = 0x4F,
    VBE_AH_SUCCESS = 0x00,

    VBE_AL_SUCCESS = 0x4F,
    VBE_AL_GET_CONTROLLER_INFO = 0x00,
    VBE_AL_GET_MODE_INFO = 0x01,
    VBE_AL_SET_MODE = 0x02,
    VBE_AL_DDC = 0x15,

    VBE_BL_DDC_REPORT = 0x00,
    VBE_BL_DDC_READ_EDID = 0x01,
};

#define VBE_MODE_LIST_BUFFER_SIZE 256
#define VBE_MODE_LIST_TERMINATOR 0xFFFF

STATUS VgaVesaGetAdapterInfo(struct VgaAdapterInfo *info)
{
    STATUS status = OK;
    struct I686Registers regs;
    struct VbeInfoBlock *vbeInfo = NULL;
    struct VbeModeInfoBlock *modeInfo = NULL;
    uint16_t *modeList = NULL;
    bool isVbe3 = false;

    info->modeCount = 0;
    
    vbeInfo = MmAllocateKernelHeap(sizeof(*vbeInfo));
    if(NULL == vbeInfo)
        return OUT_OF_RESOURCES;

    status = I686AcquireEmulator(MS_TO_NS(5000));
    if(OK != status)
    {
        MmFreeKernelHeap(vbeInfo);
        return status;
    }

    strncpy(vbeInfo->signature, "VBE2", 4);
    regs.ah = VBE_AH_DO;
    regs.al = VBE_AL_GET_CONTROLLER_INFO;
    regs.es = EMU_DATA_SEGMENT;
    regs.di = EMU_DATA_OFFSET;

    if(EMU_OK != I686EmulatorDoInterrupt(VBE_INT, &regs, vbeInfo, sizeof(*vbeInfo)))
    {
        status = DEVICE_NOT_AVAILABLE;
        goto VgeVesaGetAdapterInfoExit;
    }

    if((!!strncmp(vbeInfo->signature, "VESA", 4)) 
        || (VBE_AL_SUCCESS != regs.al) || (VBE_AH_SUCCESS != regs.ah)
        || (vbeInfo->capabilities[0] & (1 << 1)) //not VGA-compatible?
        || ((vbeInfo->version >> 8) < 2)) //version <2.x?
    {
        status = DEVICE_NOT_AVAILABLE;
        goto VgeVesaGetAdapterInfoExit;
    }

    if((vbeInfo->version >> 8) >= 3)
        isVbe3 = true;

    modeList = MmAllocateKernelHeap(VBE_MODE_LIST_BUFFER_SIZE * sizeof(*modeList));
    if(NULL == modeList)
    {
        status = OUT_OF_RESOURCES;
        goto VgeVesaGetAdapterInfoExit;
    }

    modeInfo = MmAllocateKernelHeap(sizeof(*modeInfo));
    if(NULL == modeInfo)
    {
        status = OUT_OF_RESOURCES;
        goto VgeVesaGetAdapterInfoExit;
    }

    uint32_t offset = EMU_FAR_POINTER_TO_LINEAR(vbeInfo->modeList >> 16, vbeInfo->modeList & 0xFF);
    while(1)
    {
        uint32_t max = VBE_MODE_LIST_BUFFER_SIZE;
        if(EMU_OK != I686EmulatorReadMemory(offset, VBE_MODE_LIST_BUFFER_SIZE * sizeof(*modeList), modeList))
        {
            status = DEVICE_NOT_AVAILABLE;
            goto VgeVesaGetAdapterInfoExit;
        }

        uint16_t *mode = modeList;
        while((VBE_MODE_LIST_TERMINATOR != *mode) && (0 != max--))
        {
            regs.ah = VBE_AH_DO;
            regs.al = VBE_AL_GET_MODE_INFO;
            regs.cx = *mode;
            regs.es = EMU_DATA_SEGMENT;
            regs.di = EMU_DATA_OFFSET;
            if(EMU_OK != I686EmulatorDoInterrupt(VBE_INT, &regs, modeInfo, sizeof(*modeInfo)))
            {
                status = DEVICE_NOT_AVAILABLE;
                goto VgeVesaGetAdapterInfoExit;
            }

            if((VBE_AL_SUCCESS == regs.al) && (VBE_AH_SUCCESS == regs.ah)
                && (modeInfo->attributes & (1 << 7)) //linear framebuffer available
                && (VBE_MEMORY_MODEL_DIRECT_COLOR == modeInfo->memoryModel))
            {
                size_t k = info->modeCount;
                info->mode[k].mode = *mode;
                info->mode[k].config.bitsPerPixel = modeInfo->bitsPerPixel;
                info->mode[k].config.height = modeInfo->yRes;
                info->mode[k].config.width = modeInfo->xRes;
                info->mode[k].lfb = modeInfo->lfbBase;
                info->mode[k].maxClock = isVbe3 ? modeInfo->maxPixelClock : 0;
                info->mode[k].available.doubleScan = (modeInfo->attributes & (1 << 8)) ? 1 : 0;
                info->mode[k].available.interlaced = (modeInfo->attributes & (1 << 9)) ? 1 : 0;

                if(isVbe3)
                {
                    info->mode[k].config.pitch = modeInfo->linBytesPerScanLine;
                    info->mode[k].config.mask.red.position = modeInfo->linRedPosition;
                    info->mode[k].config.mask.red.size = modeInfo->linRedMaskSize;
                    info->mode[k].config.mask.green.position = modeInfo->linGreenPosition;
                    info->mode[k].config.mask.green.size = modeInfo->linGreenMaskSize;
                    info->mode[k].config.mask.blue.position = modeInfo->linBluePosition;
                    info->mode[k].config.mask.blue.size = modeInfo->linBlueMaskSize;
                    info->mode[k].config.mask.reserved.position = modeInfo->linRsvdPosition;
                    info->mode[k].config.mask.reserved.size = modeInfo->linRsvdMaskSize;
                }
                else
                {
                    info->mode[k].config.pitch = modeInfo->bytesPerScanLine;
                    info->mode[k].config.mask.red.position = modeInfo->redPosition;
                    info->mode[k].config.mask.red.size = modeInfo->redMaskSize;
                    info->mode[k].config.mask.green.position = modeInfo->greenPosition;
                    info->mode[k].config.mask.green.size = modeInfo->greenMaskSize;
                    info->mode[k].config.mask.blue.position = modeInfo->bluePosition;
                    info->mode[k].config.mask.blue.size = modeInfo->blueMaskSize;
                    info->mode[k].config.mask.reserved.position = modeInfo->rsvdPosition;
                    info->mode[k].config.mask.reserved.size = modeInfo->rsvdMaskSize;
                }

                if(VGA_MAX_MODES == ++info->modeCount)
                    goto VgeVesaGetAdapterInfoExit;
            }


            ++mode;
        }

        offset += VBE_MODE_LIST_BUFFER_SIZE * sizeof(*modeList);
    }

VgeVesaGetAdapterInfoExit:
    I686ReleaseEmulator();
    MmFreeKernelHeap(vbeInfo);
    MmFreeKernelHeap(modeList);
    MmFreeKernelHeap(modeInfo);
    return status;
}

static void VgaVesaParseEstablishedTimings(const struct VbeEdid *edid, struct VgaDisplayInfo *info)
{
    size_t i = info->timingCount;
    if(edid->establishedTimings[0] & 0x1)
        info->timing[i++] = VgaTimings[VESA_800_600_60];
    if(edid->establishedTimings[0] & 0x2) 
        info->timing[i++] = VgaTimings[VESA_800_600_56];
    if(edid->establishedTimings[0] & 0x4) 
        info->timing[i++] = VgaTimings[VESA_640_480_75];
    if(edid->establishedTimings[0] & 0x8) 
        info->timing[i++] = VgaTimings[VESA_640_480_72];
    if(edid->establishedTimings[0] & 0x20) 
        info->timing[i++] = VgaTimings[VGA_640_480_60];

    if(edid->establishedTimings[1] & 0x1)
        info->timing[i++] = VgaTimings[VESA_1280_1024_75];
    if(edid->establishedTimings[1] & 0x2)
        info->timing[i++] = VgaTimings[VESA_1024_768_75];        
    if(edid->establishedTimings[1] & 0x4)
        info->timing[i++] = VgaTimings[VESA_1024_768_70];   
    if(edid->establishedTimings[1] & 0x8)
        info->timing[i++] = VgaTimings[VESA_1024_768_60];   
    if(edid->establishedTimings[1] & 0x40)
        info->timing[i++] = VgaTimings[VESA_800_600_75];
    if(edid->establishedTimings[1] & 0x80)
        info->timing[i++] = VgaTimings[VESA_800_600_72];      
    
    info->timingCount = i;
}

static void VgaVesaParseDetailedTiming(const struct VbeEdid *edid, size_t index, struct VgaDisplayInfo *info, bool setAsPreferred)
{
    if(0 == edid->descriptor[index].detailedTiming.pixelClock)
        return;
    
    size_t i = info->timingCount;

    info->timing[i].clock = (uint32_t)RtlLeU16(edid->descriptor[index].detailedTiming.pixelClock) * (uint32_t)10000;

    info->timing[i].porch.x = edid->descriptor[index].detailedTiming.xPorchLo;
    info->timing[i].porch.x += ((uint32_t)edid->descriptor[index].detailedTiming.xyPorchSyncHi << 2) & 0x300;
    info->timing[i].porch.y = edid->descriptor[index].detailedTiming.yPorchSyncLo >> 4;
    info->timing[i].porch.y += (edid->descriptor[index].detailedTiming.xyPorchSyncHi << 2) & 0x30;

    info->timing[i].sync.x = edid->descriptor[index].detailedTiming.xSyncLo;
    info->timing[i].sync.x += ((uint32_t)edid->descriptor[index].detailedTiming.xyPorchSyncHi << 4) & 0x300;
    info->timing[i].sync.y = edid->descriptor[index].detailedTiming.yPorchSyncLo & 0xF;
    info->timing[i].sync.y += ((uint32_t)edid->descriptor[index].detailedTiming.xyPorchSyncHi << 8) & 0x300;

    info->timing[i].usable.x = edid->descriptor[index].detailedTiming.xResLo;
    info->timing[i].usable.x += ((uint32_t)edid->descriptor[index].detailedTiming.xResBlankHi << 4) & 0xF00;
    info->timing[i].usable.y = edid->descriptor[index].detailedTiming.yResLo;
    info->timing[i].usable.y += ((uint32_t)edid->descriptor[index].detailedTiming.yResBlankHi << 4) & 0xF00;

    info->timing[i].total = info->timing[i].usable;
    info->timing[i].total.x += edid->descriptor[index].detailedTiming.xBlankLo;
    info->timing[i].total.x += ((uint32_t)edid->descriptor[index].detailedTiming.xResBlankHi << 8) & 0xF00;
    info->timing[i].total.y += edid->descriptor[index].detailedTiming.yBlankLo;
    info->timing[i].total.y += ((uint32_t)edid->descriptor[index].detailedTiming.yResBlankHi << 8) & 0xF00;

    info->timing[i].flags.interlaced = !!(edid->descriptor[index].detailedTiming.flags & 0x80);
    info->timing[i].flags.hSync = !(edid->descriptor[index].detailedTiming.flags & 0x1);
    info->timing[i].flags.vSync = !(edid->descriptor[index].detailedTiming.flags & 0x2);

    info->preferredTiming = i;

    info->timingCount = ++i;
}

STATUS VgaVesaGetDisplayInfo(struct VgaDisplayInfo *info)
{
    STATUS status = OK;
    struct I686Registers regs;
    struct VbeEdid *edid = NULL;

    strcpy(info->id, "GENVGA0");
    info->timingCount = 0;
    
    status = I686AcquireEmulator(MS_TO_NS(5000));
    if(OK != status)
        return status;

    regs.ah = VBE_AH_DO;
    regs.al = VBE_AL_DDC;
    regs.bl = VBE_BL_DDC_REPORT;
    regs.cx = 0;
    regs.dx = 0;
    regs.es = 0;
    regs.di = 0;

    if((EMU_OK != I686EmulatorDoInterrupt(VBE_INT, &regs, NULL, 0))
        || (VBE_AL_SUCCESS != regs.al) || (VBE_AH_SUCCESS != regs.ah)
        || (!(regs.bl & (0x1 | 0x2)))) //no DDC1 nor DDC2 support
    {
        goto VgaVesaGetDisplayInfoExit;
    }

    edid = MmAllocateKernelHeap(sizeof(*edid));
    if(NULL == edid)
    {
        status = OUT_OF_RESOURCES;
        goto VgaVesaGetDisplayInfoExit;
    }

    regs.ah = VBE_AH_DO;
    regs.al = VBE_AL_DDC;
    regs.bl = VBE_BL_DDC_READ_EDID;
    regs.es = EMU_DATA_SEGMENT;
    regs.di = EMU_DATA_OFFSET;
    if((EMU_OK != I686EmulatorDoInterrupt(VBE_INT, &regs, edid, sizeof(*edid)))
        || ((VBE_AL_SUCCESS != regs.al) || (VBE_AH_SUCCESS != regs.ah)))
    {
        goto VgaVesaGetDisplayInfoExit;
    }

    uint8_t checksum = 0;
    for(size_t i = 0; i < sizeof(*edid); i++)
        checksum += ((uint8_t*)edid)[i];
    
    if(0 != checksum)
    {
        goto VgaVesaGetDisplayInfoExit;
    }

    if(0x00FFFFFFFFFFFF00 != edid->header)
    {
        goto VgaVesaGetDisplayInfoExit;
    }

    //while EDID is little endian, the manufacturer field is described as if it was big endian, or at least it is easier to parse it like so
    info->id[0] = ((RtlBeU16(edid->manufacturer) >> 10) & 0x1F) + 'A';
    info->id[1] = ((RtlBeU16(edid->manufacturer) >> 5) & 0x1F) + 'A';
    info->id[2] = (RtlBeU16(edid->manufacturer) & 0x1F) + 'A';
    snprintf(&info->id[3], 5, "%4X", RtlLeU16(edid->product));
    info->id[7] = '\0';

    info->serial = RtlLeU32(edid->serial);
    info->timingCount = 0;
    info->preferredTiming = (size_t)(-1);

    VgaVesaParseEstablishedTimings(edid, info);

    //TODO: parse standard timings

    for(uint8_t i = 0; i < 4; i++)
    {
        if(0 != edid->descriptor[i].detailedTiming.pixelClock)
            VgaVesaParseDetailedTiming(edid, i, info, (0 == i));
    }

VgaVesaGetDisplayInfoExit:
    I686ReleaseEmulator();
    MmFreeKernelHeap(edid);
    return status;
}

STATUS VgaVesaSetMode(uint16_t vbeMode, const struct VgaTiming *timing)
{
    STATUS status = OK;
    struct I686Registers regs;
    struct VbeCrtc *crtc = NULL;

    if(vbeMode > 0x1FF)
        return BAD_PARAMETER;
    
    regs.ah = VBE_AH_DO;
    regs.al = VBE_AL_SET_MODE;
    regs.bx = vbeMode | (1 << 14); //use linear framebuffer bit
    regs.es = EMU_DATA_SEGMENT;
    regs.di = EMU_DATA_OFFSET;

    if(NULL != timing)
    {
        regs.bx |= (1 << 11); //use specified CRTC

        crtc = MmAllocateKernelHeapZeroed(sizeof(*crtc));
        if(NULL == crtc)
            return OUT_OF_RESOURCES;

        crtc->xTotal = timing->total.x;
        crtc->yTotal = timing->total.y;
        crtc->xSyncStart = timing->usable.x + timing->porch.x;
        crtc->ySyncStart = timing->usable.y + timing->porch.y;
        crtc->xSyncEnd = crtc->xSyncStart + timing->sync.x;
        crtc->ySyncEnd = crtc->ySyncStart + timing->sync.y;
        
        crtc->clock = timing->clock;
        crtc->refresh = ((uint64_t)100 * (uint64_t)timing->clock) / (uint64_t)((uint32_t)timing->total.x * (uint32_t)timing->total.y);

        if(timing->flags.doubleScan)
            crtc->flags |= (1 << 0);
        if(timing->flags.interlaced)
            crtc->flags |= (1 << 1);
        if(timing->flags.hSync)
            crtc->flags |= (1 << 2);
        if(timing->flags.vSync)
            crtc->flags |= (1 << 3);
    }

    status = I686AcquireEmulator(MS_TO_NS(5000));
    if(OK != status)
    {
        MmFreeKernelHeap(crtc);
        return status;
    }

    if((EMU_OK != I686EmulatorDoInterrupt(VBE_INT, &regs, crtc, (NULL != crtc) ? sizeof(*crtc) : 0))
        || (VBE_AL_SUCCESS != regs.al) || (VBE_AH_SUCCESS != regs.ah))
    {
        status = UNKNOWN_ERROR;
    }

    I686ReleaseEmulator();
    MmFreeKernelHeap(crtc);
    return status;    
}