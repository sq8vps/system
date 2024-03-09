#include "device.h"
#include "io/dev/dev.h"
#include "mm/heap.h"
#include "common.h"
#include "logging.h"
#include "ke/core/dpc.h"
#include "ata.h"
#include "config.h"

#define IDE_DEVICE_ID_PREFIX "DISK"
#define IDE_DEVICE_ID_GENERIC "GENERIC"

#define IDE_DMA_REQUIRED_ALIGNMENT 2

/**
 * @brief Request Packet callback called by IO queue manager
*/
void IdeProcessRequest(struct IoRp *rp)
{
    STATUS status = OK;
    struct IdeDriveData *info = &(((struct IdeDeviceData*)(IoGetCurrentRpPosition(rp)->privateData))->drive);

    info->controller->channel[info->channel].rp = rp;

    switch(rp->code)
    {
        case IO_RP_READ:
            status = IdeReadWrite(info, false, rp->payload.read.offset, rp->size, rp->payload.read.memory);
            break;
        case IO_RP_WRITE:
            status = IdeReadWrite(info, true, rp->payload.write.offset, rp->size, rp->payload.write.memory);
            break;
        default:
            status = IO_RP_CODE_UNKNOWN;
            break;
    }
    
    if(OK != status)
    {
        //finalize immediately on failure
        rp->status = status;
        IoFinalizeRp(rp);
    }
}

/**
 * @brief Request finalization callback, handled as DPC registered from ISR
*/
static void IdeFinalizeRequest(void *context)
{
    IoFinalizeRp((struct IoRp*)context);
}

/**
 * @brief Create new disk drive device
 * 
 * This function is called when the IDE controller is enumerating the bus
*/
STATUS IdeCreateDriveDevice(struct IdeDeviceData *deviceData, struct IoDeviceObject *enumerator, struct ExDriverObject *driver)
{
    if((NULL == deviceData) || (NULL == driver))
        return NULL_POINTER_GIVEN;

    STATUS status;
    
    struct IoDeviceObject *dev;
    if(OK != (status = IoCreateDevice(driver, IO_DEVICE_TYPE_STORAGE, IO_DEVICE_FLAG_DIRECT_IO, &dev)))
    {
        return status;
    }

    struct IdeDriveData *drive = &(deviceData->drive);

    dev->blockSize = drive->sectorSize;
    dev->alignment = IDE_DMA_REQUIRED_ALIGNMENT;
    dev->flags |= IO_DEVICE_FLAG_DIRECT_IO;

    //register new device in OS
    if(OK != (status = IoRegisterDevice(dev, enumerator)))
    {
        return status;
    }

    LOG(SYSLOG_INFO, "Registered disk drive (%s)", drive->model);

    dev->privateData = deviceData;

    return OK;
}

STATUS IdeCreateAllDriveDevices(struct IdeControllerData *info, struct IoDeviceObject *dev, struct ExDriverObject *driver)
{
    if(NULL != info->channel[PCI_IDE_CHANNEL_PRIMARY].drive[PCI_IDE_SLOT_MASTER])
        IdeCreateDriveDevice(info->channel[PCI_IDE_CHANNEL_PRIMARY].drive[PCI_IDE_SLOT_MASTER], dev, driver);
    if(NULL != info->channel[PCI_IDE_CHANNEL_PRIMARY].drive[PCI_IDE_SLOT_SLAVE])
        IdeCreateDriveDevice(info->channel[PCI_IDE_CHANNEL_PRIMARY].drive[PCI_IDE_SLOT_SLAVE], dev, driver);
    if(NULL != info->channel[PCI_IDE_CHANNEL_SECONDARY].drive[PCI_IDE_SLOT_MASTER])
        IdeCreateDriveDevice(info->channel[PCI_IDE_CHANNEL_SECONDARY].drive[PCI_IDE_SLOT_MASTER], dev, driver);
    if(NULL != info->channel[PCI_IDE_CHANNEL_SECONDARY].drive[PCI_IDE_SLOT_SLAVE])
        IdeCreateDriveDevice(info->channel[PCI_IDE_CHANNEL_SECONDARY].drive[PCI_IDE_SLOT_SLAVE], dev, driver);
    return OK;
}

static STATUS IdeStartReadWrite(struct IdeDriveData *info, bool write, uint64_t lba, uint64_t size, struct MmMemoryDescriptor *buffer)
{
    KeAcquireSpinlock(&(info->controller->channel[info->channel].lock));
    //clear Physical Region Descriptors
    IdeClearPrdTable(&(info->controller->channel[info->channel].prdt));
    //form PRD entries for this operation using provided memory descriptors
    uint64_t remainingBytes = size;
    uint64_t byteLimit = info->sectorSize * (info->lba48 ? 0x10000 : 0x100); 
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
                prdFull = (0 == IdeAddPrdEntry(&(info->controller->channel[info->channel].prdt), blockNextAddress, 0));
                bytes -= IDE_PRD_MAX_SIZE;
                blockNextAddress += IDE_PRD_MAX_SIZE;
                blockRemainingBytes -= IDE_PRD_MAX_SIZE;
                remainingBytes -= IDE_PRD_MAX_SIZE;
            }
            else
            {
                prdFull = (0 == IdeAddPrdEntry(&(info->controller->channel[info->channel].prdt), blockNextAddress, bytes));
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
            info->controller->channel[info->channel].operation.memory.physical = blockNextAddress;
            info->controller->channel[info->channel].operation.memory.size = blockRemainingBytes;
            info->controller->channel[info->channel].operation.memory.next = buffer->next;
        }
        else
        {
            info->controller->channel[info->channel].operation.memory = *(buffer->next);
        }
        info->controller->channel[info->channel].operation.remaining = remainingBytes;
    }
    else
        info->controller->channel[info->channel].operation.remaining = 0;
    
    info->controller->channel[info->channel].operation.lba = lba;
    info->controller->channel[info->channel].operation.size = size;
    info->controller->channel[info->channel].operation.slot = info->drive;
    info->controller->channel[info->channel].operation.write = (true == write);
    info->controller->channel[info->channel].operation.busy = 1;
    KeReleaseSpinlock(&(info->controller->channel[info->channel].lock));

    //store PRD table
    IdeWriteBmrPrdt(info->controller, info->channel, &(info->controller->channel[info->channel].prdt));
    //clear bus master status
    uint8_t status = IdeReadBmrStatus(info->controller, info->channel);
    status |= IDE_BMR_STATUS_INTERRUPT | IDE_BMR_STATUS_ERROR;
    IdeWriteBmrStatus(info->controller, info->channel, status);

    //set operation parameters
    if(info->lba48)
        IdeWriteLba48Parameters(info->controller, info->channel, info->drive, lba, (size - remainingBytes) / info->sectorSize);
    else
        IdeWriteLba28Parameters(info->controller, info->channel, info->drive, lba, (size - remainingBytes) / info->sectorSize);

    //issue ATA command
    IdeStartTransfer(info->controller, info->channel, info->drive, write, true == info->lba48);

    //start DMA
    IdeWriteBmrCommand(info->controller, info->channel, IDE_BMR_COMMAND_START | (write ? 0 : IDE_BMR_COMMAND_RW_CONTROL));
    return OK;
}

STATUS IdeReadWrite(struct IdeDriveData *info, bool write, uint64_t offset, uint64_t size, struct MmMemoryDescriptor *buffer)
{
    //verify request
    if(!info->present || !info->usable)
        return DEVICE_NOT_AVAILABLE;

    if(size % info->sectorSize)
        return BAD_ALIGNMENT;

    if(offset % info->sectorSize)
        return BAD_ALIGNMENT;

    uint64_t availableMemory = 0;
    if(NULL != buffer)
    {
        struct MmMemoryDescriptor *t = buffer;
        while(t)
        {
            //block size must be a multiple of sector size and must be word aligned
            if((t->size % info->sectorSize)
               || (t->size & 1))
                return BAD_ALIGNMENT;
            //address must be word aligned
            if(t->physical & 1)
                return BAD_ALIGNMENT;
            //memory region must fit in 32 bits
            if((t->physical + t->size) > 0x100000000)
                return SYSTEM_INCOMPATIBLE;
            //find closest 64 KiB boundary
            uint32_t closestBoundary = (t->physical & 0xFFFF0000) + 0x10000;
            //check if the distance between the memory base and the closest boundary is a multiple of sector size
            //if so, then all sectors are aligned to 64 KiB boundary (assuming sector size being 2^n)
            if((closestBoundary - t->physical) % info->sectorSize)
                return BAD_ALIGNMENT;
            availableMemory += t->size;
            t = t->next;
        }
    }
    else
        return NULL_POINTER_GIVEN;
    
    if(availableMemory < size)
        return OUT_OF_RESOURCES;
    
    return IdeStartReadWrite(info, write, offset / info->sectorSize, size, buffer);
}

STATUS IdeIsr(void *context)
{
    struct IdeControllerData *info = context;
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
                    //TODO: this was actually not tested
                    if(info->channel[i].operation.remaining > 0)
                    {
                        IdeStartReadWrite(&(info->channel[i].drive[info->channel[i].operation.slot]->drive),
                            info->channel[i].operation.write, 
                            info->channel[i].operation.lba
                                + ((info->channel[i].operation.size - info->channel[i].operation.remaining) 
                                    / info->channel[i].drive[info->channel[i].operation.slot]->drive.sectorSize),
                            info->channel[i].operation.remaining,
                            &(info->channel[i].operation.memory)
                        );
                    }
                    else //all data transferred
                    {
                        KeAcquireSpinlock(&(info->channel[i].lock));
                        //RP finalization
                        info->channel[i].rp->status = OK;
                        KeRegisterDpc(KE_DPC_PRIORITY_NORMAL, IdeFinalizeRequest, info->channel[i].rp);
                        CmMemset(&(info->channel[i].operation), 0, sizeof(info->channel[i].operation));
                        info->channel[i].operation.busy = 0;
                        KeReleaseSpinlock(&(info->channel[i].lock));
                    }
                    
                }
                else //failure
                {
                    KeAcquireSpinlock(&(info->channel[i].lock));
                    //RP finalization
                    info->channel[i].rp->status = UNKNOWN_ERROR;
                    info->channel[i].rp->size = 0;
                    KeRegisterDpc(KE_DPC_PRIORITY_NORMAL, IdeFinalizeRequest, info->channel[i].rp);
                    CmMemset(&(info->channel[i].operation), 0, sizeof(info->channel[i].operation));
                    info->channel[i].operation.busy = 0;
                    KeReleaseSpinlock(&(info->channel[i].lock));
                }
            }
            else
                KeReleaseSpinlock(&(info->channel[i].lock));
        }
    }
    return OK;
}

STATUS IdeGetDeviceId(struct IoRp *rp)
{
    struct IoDeviceObject *dev = IoGetCurrentRpPosition(rp);

    struct IdeDeviceData *t = dev->privateData;
    //if it is a MDO, then it is the controller. Ask BDO for device ID
    if(t->isController)
    {
        return IoSendRpDown(rp);
    }
    struct IdeDriveData *info = &(t->drive);

    char *deviceId, **compatibleIds;

    deviceId = ExMakeDeviceId(2, IDE_DEVICE_ID_PREFIX, info->model);

    if(NULL == deviceId)
    {
        rp->status = OUT_OF_RESOURCES;
        goto _IdeGetDeviceIdLeave;
    }

    compatibleIds = MmAllocateKernelHeapZeroed(IO_MAX_COMPATIBLE_DEVICE_IDS * sizeof(*compatibleIds));
    if(NULL == compatibleIds)
    {
        MmFreeKernelHeap(deviceId);
        rp->status = OUT_OF_RESOURCES;
        goto _IdeGetDeviceIdLeave;
    }

    compatibleIds[0] = ExMakeDeviceId(2, IDE_DEVICE_ID_PREFIX, IDE_DEVICE_ID_GENERIC);

    rp->payload.deviceId.mainId = deviceId;
    rp->payload.deviceId.compatibleId = compatibleIds;
    rp->status = OK;

_IdeGetDeviceIdLeave:
    IoFinalizeRp(rp);
    return OK;
}

STATUS IdeStorageControl(struct IoRp *rp)
{
    struct IoDeviceObject *dev = IoGetCurrentRpPosition(rp);

    struct IdeDeviceData *t = dev->privateData;

    switch(rp->payload.deviceControl.code)
    {
        case STOR_GET_GEOMETRY:
            if(t->isController)
            {
                rp->status = IO_RP_PROCESSING_FAILED;
                break;
            }
            struct IdeDriveData *info = &(t->drive);
            rp->payload.deviceControl.data = MmAllocateKernelHeapZeroed(sizeof(struct StorGeometry));
            if(NULL == rp->payload.deviceControl.data)
            {
                rp->status = OUT_OF_RESOURCES;
                break;
            }
            struct StorGeometry *geo = rp->payload.deviceControl.data;
            geo->sectorSize = info->sectorSize;
            geo->sectorCount = info->sectors;
            geo->firstAddressableSector = 0;
            rp->status = OK;
            break;
        
        default:
            rp->status = IO_RP_CODE_UNKNOWN;
            break;
    }
 
    IoFinalizeRp(rp);
    return OK;    
}