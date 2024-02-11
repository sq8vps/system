#include "device.h"
#include "io/dev/dev.h"
#include "mm/heap.h"
#include "common.h"
#include "logging.h"
#include "ke/core/dpc.h"

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



static STATUS IdeStartReadWrite(struct IdeDeviceData *info, uint8_t channel, uint8_t slot, bool write, uint64_t lba, uint64_t size, struct IoMemoryDescriptor *buffer)
{
    KeAcquireSpinlock(&(info->channel[channel].lock));
    //clear Physical Region Descriptors
    IdeClearPrdTable(&(info->channel[channel].prdt));
    //form PRD entries for this operation using provided memory descriptors
    uint64_t remainingBytes = size;
    uint64_t byteLimit = info->channel[channel].drive[slot].sectorSize * (info->channel[channel].drive[slot].lba48 ? 0x10000 : 0x100); 
    uint64_t blockRemainingBytes = buffer->size;
    uint64_t blockNextAddress = buffer->physical;
    while(blockRemainingBytes > 0)
    {
        if((size - remainingBytes) >= byteLimit)
            goto IdeStartReadWriteContinue;
        uint64_t bytes = remainingBytes;
        if(bytes > blockRemainingBytes)
            bytes = blockRemainingBytes;
        if(bytes > byteLimit)
            bytes = byteLimit;
        while(bytes > 0)
        {
            bool prdFull = false;
            if(bytes > IDE_PRD_MAX_SIZE)
            {
                //0 in PRD size means 65536 bytes
                prdFull = (0 == IdeAddPrdEntry(&(info->channel[channel].prdt), blockNextAddress, 0));
                bytes -= IDE_PRD_MAX_SIZE;
                blockNextAddress += IDE_PRD_MAX_SIZE;
                blockRemainingBytes -= IDE_PRD_MAX_SIZE;
                remainingBytes -= IDE_PRD_MAX_SIZE;
            }
            else
            {
                prdFull = (0 == IdeAddPrdEntry(&(info->channel[channel].prdt), blockNextAddress, bytes));
                blockNextAddress += bytes;
                blockRemainingBytes -= bytes;
                remainingBytes -= bytes;
                bytes = 0;
            }
            if(prdFull)
                goto IdeStartReadWriteContinue;
        }
        if(0 == remainingBytes)
            break;
        buffer = buffer->next;
        blockRemainingBytes = buffer->size;
        blockNextAddress = buffer->physical;
    }

IdeStartReadWriteContinue:
    if(remainingBytes > 0)
    {
        if(blockRemainingBytes > 0)
        {
            info->channel[channel].operation.memory.physical = blockNextAddress;
            info->channel[channel].operation.memory.size = blockRemainingBytes;
            info->channel[channel].operation.memory.next = buffer->next;
        }
        else
        {
            info->channel[channel].operation.memory = *(buffer->next);
        }
        info->channel[channel].operation.remaining = remainingBytes;
    }
    else
        info->channel[channel].operation.remaining = 0;
    
    info->channel[channel].operation.lba = lba;
    info->channel[channel].operation.size = size;
    info->channel[channel].operation.slot = slot;
    info->channel[channel].operation.write = (true == write);
    info->channel[channel].operation.busy = 1;
    KeReleaseSpinlock(&(info->channel[channel].lock));

    //store PRD table
    IdeWriteBmrPrdt(info, channel, &(info->channel[channel].prdt));
    //clear bus master status
    uint8_t status = IdeReadBmrStatus(info, channel);
    status |= IDE_BMR_STATUS_INTERRUPT | IDE_BMR_STATUS_ERROR;
    IdeWriteBmrStatus(info, channel, status);

    //set operation parameters
    if(info->channel[channel].drive[slot].lba48)
        IdeWriteLba48Parameters(info, channel, slot, lba, (size - remainingBytes) / info->channel[channel].drive[slot].sectorSize);
    else
        IdeWriteLba28Parameters(info, channel, slot, lba, (size - remainingBytes) / info->channel[channel].drive[slot].sectorSize);

    //issue ATA command
    IdeStartTransfer(info, channel, slot, write, true == info->channel[channel].drive[slot].lba48);

    //start DMA
    IdeWriteBmrCommand(info, channel, IDE_BMR_COMMAND_START | (write ? 0 : IDE_BMR_COMMAND_RW_CONTROL));
    return OK;
}

STATUS IdeReadWrite(struct IdeDeviceData *info, uint8_t channel, uint8_t slot, bool write, uint64_t lba, uint64_t size, struct IoMemoryDescriptor *buffer)
{
    //verify request
    if(!info->channel[channel].drive[slot].present || !info->channel[channel].drive[slot].usable)
        return DEVICE_NOT_AVAILABLE;

    if(size % info->channel[channel].drive[slot].sectorSize)
        return BAD_ALIGNMENT;

    uint64_t availableMemory = 0;
    if(NULL != buffer)
    {
        struct IoMemoryDescriptor *t = buffer;
        while(t)
        {
            //block size must be a multiple of sector size and must be even
            if((t->size % info->channel[channel].drive[slot].sectorSize)
               || (t->size & 1))
                return BAD_ALIGNMENT;
            //address must be even
            if(t->physical & 1)
                return BAD_ALIGNMENT;
            //memory region must fit in 32 bits
            if((t->physical + t->size) > 0x100000000)
                return SYSTEM_INCOMPATIBLE;
            //find closest 64 KiB boundary
            uint32_t closestBoundary = (t->physical & 0xFFFF0000) + 0x10000;
            //check if the distance between the memory base and the closest boundary is a multiple of sector size
            //if so, then all sectors are aligned to 64 KiB boundary (assuming sector size being 2^n)
            if((closestBoundary - t->physical) % info->channel[channel].drive[slot].sectorSize)
                return BAD_ALIGNMENT;
            availableMemory += t->size;
            t = t->next;
        }
    }
    else
        return NULL_POINTER_GIVEN;
    
    if(availableMemory < size)
        return OUT_OF_RESOURCES;
    
    return IdeStartReadWrite(info, channel, slot, write, lba, size, buffer);
}

STATUS IdeIsr(void *context)
{
    struct IdeDeviceData *info = context;
    for(uint8_t i = 0; i < 2; i++)
    {
        uint8_t bmrStatus = IdeReadBmrStatus(info, i);
        if(bmrStatus & IDE_BMR_STATUS_INTERRUPT)
        {
            IdeWriteBmrCommand(info, i, 0);
            IdeWriteBmrStatus(info, i, bmrStatus | IDE_BMR_STATUS_INTERRUPT | IDE_BMR_STATUS_ERROR);
            KeAcquireSpinlock(&(info->channel[i].lock));
            if(info->channel[i].operation.busy)
            {
                KeReleaseSpinlock(&(info->channel[i].lock));
                //after successful completion the activity bit must be cleared. Of course the error bit must be cleared too.
                if(!(bmrStatus & IDE_BMR_STATUS_ACTIVE) && !(bmrStatus & IDE_BMR_STATUS_ERROR))
                {
                    //more data to transfer
                    // if(info->channel[i].operation.remaining > 0)
                    // {
                    //     IdeStartReadWrite(info, i, 
                    //         info->channel[i].operation.slot, 
                    //         info->channel[i].operation.write, 
                    //         info->channel[i].operation.lba
                    //             + ((info->channel[i].operation.size - info->channel[i].operation.remaining) 
                    //                 / info->channel[i].drive[info->channel[i].operation.slot].sectorSize),
                    //         info->channel[i].operation.remaining,
                    //         &(info->channel[i].operation.memory)
                    //     );
                    // }
                    // else //all data transferred
                    // {
                    //     KeAcquireSpinlock(&(info->channel[i].lock));
                    //     //RP finalization

                    //     CmMemset(&(info->channel[i].operation), 0, size(info->channel[i].operation));
                    //     info->channel[i].operation.busy = 0;
                    //     KeReleaseSpinlock(&(info->channel[i].lock));
                    // }
                    LOG(SYSLOG_ERROR, "Operation successful");
                }
                else
                {
                    LOG(SYSLOG_ERROR, "Operation failed");
                }
            }
            else
                KeReleaseSpinlock(&(info->channel[i].lock));
        }
    }
    return OK;
}