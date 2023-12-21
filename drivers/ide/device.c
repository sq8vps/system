#include "device.h"
#include "io/dev/dev.h"
#include "mm/heap.h"
#include "common.h"
#include "logging.h"

#define IDE_DEVICE_ID_PREFIX "SCSI"
#define IDE_DEVICE_ID_GENERIC "DISK"

STATUS IdeCreateDriveDevice(struct IdeDriveData *drive, struct ExDriverObject *driver)
{
    if((NULL == drive) || (NULL == driver))
        return NULL_POINTER_GIVEN;
    if(0 == drive->present)
        return DEVICE_NOT_AVAILABLE;

    STATUS status;
    //create subdevice for new device
    struct IoSubDeviceObject *dev;
    if(OK != (status = IoCreateSubDevice(driver, 0, &dev)))
    {
        return status;
    }

    //register new device in OS
    char *deviceId = ExMakeDeviceId(2, IDE_DEVICE_ID_PREFIX, drive->model);
    if(OK != (status = IoRegisterDevice(dev, deviceId)))
    {
        MmFreeKernelHeap(deviceId);
        return status;
    }

    LOG(SYSLOG_INFO, "Registered disk drive (%s)", deviceId);

    MmFreeKernelHeap(deviceId);

    deviceId = ExMakeDeviceId(2, IDE_DEVICE_ID_PREFIX, IDE_DEVICE_ID_GENERIC);
    if(NULL != deviceId)
    {
        IoUpdateCompatibleDeviceIdList(dev->mainDeviceObject, deviceId);
        MmFreeKernelHeap(deviceId);
    }

    // if(NULL == (dev->privateData = MmAllocateKernelHeap(sizeof(*drive))))
    //     return OUT_OF_RESOURCES;
    
    // CmMemcpy(dev->privateData, drive, sizeof(*drive));
    dev->privateData = drive;

    IoSetDeviceDisplayedName(dev, "Generic disk drive");

    return IoInitializeDevice(dev->mainDeviceObject);
}

STATUS IdeCreateAllDriveDevices(struct IdeDeviceData *info, struct ExDriverObject *driver)
{
    if(info->channel[PCI_IDE_CHANNEL_PRIMARY].drive[PCI_IDE_SLOT_MASTER].present)
        IdeCreateDriveDevice(&(info->channel[PCI_IDE_CHANNEL_PRIMARY].drive[PCI_IDE_SLOT_MASTER]), driver);
    if(info->channel[PCI_IDE_CHANNEL_PRIMARY].drive[PCI_IDE_SLOT_SLAVE].present)
        IdeCreateDriveDevice(&(info->channel[PCI_IDE_CHANNEL_PRIMARY].drive[PCI_IDE_SLOT_SLAVE]), driver);
    if(info->channel[PCI_IDE_CHANNEL_SECONDARY].drive[PCI_IDE_SLOT_MASTER].present)
        IdeCreateDriveDevice(&(info->channel[PCI_IDE_CHANNEL_SECONDARY].drive[PCI_IDE_SLOT_MASTER]), driver);
    if(info->channel[PCI_IDE_CHANNEL_SECONDARY].drive[PCI_IDE_SLOT_SLAVE].present)
        IdeCreateDriveDevice(&(info->channel[PCI_IDE_CHANNEL_SECONDARY].drive[PCI_IDE_SLOT_SLAVE]), driver);
    return OK;
}