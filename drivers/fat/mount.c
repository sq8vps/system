#include "mount.h"
#include "io/dev/vol.h"
#include "io/dev/rp.h"
#include "io/dev/op.h"
#include "structs.h"
#include "rtl/order.h"
#include "mm/heap.h"

#include "logging.h"

#define FAT12_CLUSTER_COUNT_LIMIT 4084
#define FAT16_CLUSTER_COUNT_LIMIT 65524

#define FAT_EXTENDED_BOOT_SIGNATURE_FLAG 0x29

STATUS FatMount(struct ExDriverObject *drv, struct IoDeviceObject *disk)
{
    STATUS status = OK;
    struct FatBpb *bpb = NULL;
    struct IoDeviceObject *dev = NULL;

    //read BIOS Parameter Block, which sits in the first partition sector
    status = IoReadDeviceSync(disk, 0, disk->blockSize, (void**)&bpb);
    if(OK != status)
        return status;

    bpb->bytesPerSector = RtlLeU16(bpb->bytesPerSector);
    
    if((RtlLeU16(bpb->signature) != FAT_BPB_SIGNATURE_VALUE)
        || (0 == bpb->bytesPerSector)
        || (0 == bpb->sectorsPerCluster))
    {
        MmFreeKernelHeap(bpb);
        return IO_UNKNOWN_FILE_SYSTEM;
    }

    status = IoCreateDevice(drv, IO_DEVICE_TYPE_FS, 0, &dev);
    if(OK != status)
    {
        MmFreeKernelHeap(bpb);
        return status;
    }

    if(disk->flags & IO_DEVICE_FLAG_DIRECT_IO)
        dev->flags |= IO_DEVICE_FLAG_DIRECT_IO;
    if(disk->flags & IO_DEVICE_FLAG_BUFFERED_IO)
        dev->flags |= IO_DEVICE_FLAG_BUFFERED_IO;
    
    dev->alignment = disk->alignment;
    dev->blockSize = disk->blockSize;

    dev->privateData = MmAllocateKernelHeapZeroed(sizeof(struct FatVolume));
    if(NULL == dev->privateData)
    {
        IoDestroyDevice(dev);
        MmFreeKernelHeap(bpb);
        return OUT_OF_RESOURCES;
    }

    struct FatVolume *info = dev->privateData;

    bpb->rootEntryCount = RtlLeU16(bpb->rootEntryCount);
    bpb->reservedSectors = RtlLeU16(bpb->reservedSectors);

    //determine FAT type as specified by Microsoft
    uint64_t rootDirSectors = ((bpb->rootEntryCount * 32) + (bpb->bytesPerSector - 1) / bpb->bytesPerSector);
    uint32_t fatSize = bpb->fatSize16 ? RtlLeU16(bpb->fatSize16) : RtlLeU32(bpb->ext32.fatSize32);
    uint32_t sectors = bpb->sectors16 ? RtlLeU16(bpb->sectors16) : RtlLeU32(bpb->sectors32);
    uint32_t dataSectors = sectors - (bpb->reservedSectors + (bpb->fatCount * fatSize) + rootDirSectors);
    uint32_t clusterCount = dataSectors / bpb->sectorsPerCluster;

    if(clusterCount <= FAT12_CLUSTER_COUNT_LIMIT)
        info->type = FAT12;
    else if(clusterCount <= FAT16_CLUSTER_COUNT_LIMIT)
        info->type = FAT16;
    else
        info->type = FAT32;
    
    info->disk = disk;
    info->vol = dev;

    info->clusters = clusterCount;
    info->fatCount = bpb->fatCount;
    info->fatSize = fatSize;
    info->sectorsPerCluster = bpb->sectorsPerCluster;
    info->reservedSectors = bpb->reservedSectors;

    info->fsInfo.leadSignature = FAT_FS_INFO_LEAD_SIGNATURE;
    info->fsInfo.strucSignature = FAT_FS_INFO_STRUC_SIGNATURE;
    info->fsInfo.trailSignature = FAT_FS_INFO_TRAIL_SIGNATURE;
    info->fsInfo.nextFree = 0xFFFFFFFF;
    info->fsInfo.freeCount = 0xFFFFFFFF;

    if((FAT12 == info->type) || (FAT16 == info->type))
    {
        info->rootEntryCount = bpb->rootEntryCount;

        if(FAT_EXTENDED_BOOT_SIGNATURE_FLAG == bpb->ext16.bootSignature)
        {
            info->serial = RtlLeU32(bpb->ext16.serial);
            RtlMemcpy(info->label, bpb->ext16.label, 11);
        }
    }
    else
    {
        info->rootCluster = RtlLeU32(bpb->ext32.rootCluster);
        info->fsInfoSector = RtlLeU16(bpb->ext32.fsInfo);

        if(FAT_EXTENDED_BOOT_SIGNATURE_FLAG == bpb->ext32.bootSignature)
        {
            info->serial = RtlLeU32(bpb->ext32.serial);
            RtlMemcpy(info->label, bpb->ext32.label, 11);
        }
    }

    MmFreeKernelHeap(bpb);

    if((FAT32 == info->type) && (0 != info->fsInfoSector))
    {
        struct FatFsInfo *fsInfo = NULL;
        status = IoReadDeviceSync(disk, info->fsInfoSector * disk->blockSize, disk->blockSize, (void**)&fsInfo);
        if(OK == status)
        {
            if((FAT_FS_INFO_LEAD_SIGNATURE == RtlLeU32(fsInfo->leadSignature))
                && (FAT_FS_INFO_STRUC_SIGNATURE == RtlLeU32(fsInfo->strucSignature))
                && (FAT_FS_INFO_LEAD_SIGNATURE == RtlLeU32(fsInfo->leadSignature)))
            {
                RtlMemcpy(&(info->fsInfo), fsInfo, sizeof(*fsInfo));

                //TODO: verify FSINFO
            }

            MmFreeKernelHeap(fsInfo);
        }
    }

    info->fat = NULL;
    info->fatLock = (KeSpinlock)KeSpinlockInitializer;
    info->modifiedClusterLow = UINT32_MAX;
    info->modifiedClusterHigh = 0;

    status = IoReadDeviceSync(disk, info->reservedSectors * disk->blockSize, info->fatSize * disk->blockSize, &(info->fat));
    if(OK != status)
    {
        IoDestroyDevice(disk);
        MmFreeKernelHeap(info->fat);
        MmFreeKernelHeap(info);
        return status;
    }

    LOG(SYSLOG_INFO, "FAT volume found");

    return IoRegisterFilesystem(disk, dev, 0);
}

STATUS FatVerify(struct ExDriverObject *drv, struct IoDeviceObject *disk)
{
    STATUS status = OK;
    struct FatBpb *bpb = NULL;

    //read BIOS Parameter Block, which sits in the first partition sector
    status = IoReadDeviceSync(disk, 0, disk->blockSize, (void**)&bpb);
    if(OK != status)
        return status;

    bpb->bytesPerSector = RtlLeU16(bpb->bytesPerSector);
    
    if((RtlLeU16(bpb->signature) != FAT_BPB_SIGNATURE_VALUE)
        || (0 == bpb->bytesPerSector)
        || (0 == bpb->sectorsPerCluster))
    {
        MmFreeKernelHeap(bpb);
        return IO_UNKNOWN_FILE_SYSTEM;
    }
    else
    {
        MmFreeKernelHeap(bpb);
        return OK;
    }
}