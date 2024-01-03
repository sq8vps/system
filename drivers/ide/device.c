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

STATUS IdeIsr(void *context)
{
    struct IdeDeviceData *info = context;

    
    return OK;
}

static STATUS IdeReadWrite(struct IdeDeviceData *info, uint8_t channel, uint8_t slot, bool write, uint64_t lba, uint64_t size, struct IoMemoryDescriptor *buffer)
{
    KeAcquireSpinlock(&(info->channel[channel].lock));
    IdeClearPrdTable(&(info->channel[channel].prdt));
    struct IoMemoryDescriptor *mem = buffer;
    uint64_t bytesProcessed = 0;
    while(bytesProcessed < size)
    {

    }
IdeReadWriteContinue:
    info->channel[channel].nextLba = lba + (bytesProcessed / info->channel[channel].drive[slot].sectorSize);
    info->channel[channel].remainingBytes = size - bytesProcessed;
    KeReleaseSpinlock(&(info->channel[channel].lock));
    IdeWriteBmrPrdt(info, channel, &(info->channel[channel].prdt));
    uint8_t status = IdeReadBmrStatus(info, channel);
    status &= ~(IDE_BMR_STATUS_INTERRUPT | IDE_BMR_STATUS_ERROR);
    IdeWriteBmrStatus(info, channel, status);

    if(info->channel[channel].drive[slot].lba48)
        IdeWriteLba48Parameters(info, channel, slot, lba, bytesProcessed / info->channel[channel].drive[slot].sectorSize);
    else
        IdeWriteLba28Parameters(info, channel, slot, lba, bytesProcessed / info->channel[channel].drive[slot].sectorSize);

    IdeStartTransfer(info, channel, slot, write, info->channel[channel].drive[slot].lba48);

    IdeWriteBmrCommand(info, channel, IDE_BMR_COMMAND_START | (write ? 0 : IDE_BMR_COMMAND_RW_CONTROL));
    return OK;
}