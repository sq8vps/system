#include "kernel.h"
#include "logging.h"
#include "disk.h"
#include "part.h"

static STATUS DiskDispatch(struct IoRp *rp)
{
    if(IoGetCurrentRpPosition(rp) == rp->device)
    {
        struct DiskData *info = IoGetCurrentRpPosition(rp)->privateData;
        if(info->isMdo && !info->isPartition0)
        {
            return IoSendRpDown(rp);
        }
        switch(rp->code)
        {
            case IO_RP_READ:
            case IO_RP_WRITE:
                return DiskReadWrite(rp);
                break;
            default:
                rp->status = IO_RP_CODE_UNKNOWN;
                IoFinalizeRp(rp);
                return IO_RP_CODE_UNKNOWN;
                break;
        }
    }
    return OK;
}

static STATUS DiskInit(struct ExDriverObject *driverObject)
{
    return OK;
} 

/**
 * @brief Add disk device object
 * 
 * This function is called for the creation of the MDO for disk (partition 0) object or for partition object
*/
static STATUS DiskAddDevice(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject)
{
    struct IoDeviceObject *device;
    STATUS status;
    if(OK != (status = IoCreateDevice(driverObject, IO_DEVICE_TYPE_DISK, 0, &device)))
        return status;
    
    if(NULL == (device->privateData = MmAllocateKernelHeapZeroed(sizeof(struct DiskData))))
        return OUT_OF_RESOURCES;

    struct DiskData *info = device->privateData;
    info->isMdo = 1; //we definitely are a MDO

    device->alignment = baseDeviceObject->alignment;
    device->blockSize = baseDeviceObject->blockSize;
    if(baseDeviceObject->flags & IO_DEVICE_FLAG_BUFFERED_IO)
        device->flags |= IO_DEVICE_FLAG_BUFFERED_IO;
    if(baseDeviceObject->flags & IO_DEVICE_FLAG_DIRECT_IO)
        device->flags |= IO_DEVICE_FLAG_DIRECT_IO;

    //we were enumerated by a disk BDO, so we are a partition
    if(IO_DEVICE_TYPE_DISK == baseDeviceObject->type)
    {
        info->isPartition0 = 0;
        //represent a dummy pass-through device and pass RPs to underlying BDO
    }
    else if(IO_DEVICE_TYPE_STORAGE == baseDeviceObject->type) //if enumerated by the storage driver, then we are a partition 0 (flat disk)
    {
        info->isPartition0 = 1;
        struct StorGeometry *geo;
        status = StorGetGeometry(baseDeviceObject, &geo);
        if(OK != status)
        {
            MmFreeKernelHeap(device->privateData);
            return status;
        }
        info->partition.start = geo->firstAddressableSector;
        info->partition.size = geo->sectorCount;
        info->partition.sectorSize = geo->sectorSize;
        MmFreeKernelHeap(geo);
        status = DiskInitializeVolume(baseDeviceObject, info);
        if(OK != status)
            return status;
    }
    else //incorrect scenario
    {
        MmFreeKernelHeap(device->privateData);
        return DEVICE_NOT_AVAILABLE;
    }
        
    IoAttachDevice(device, baseDeviceObject);

    return OK;
}

STATUS DRIVER_ENTRY(struct ExDriverObject *driverObject)
{
    driverObject->init = DiskInit;
    driverObject->dispatch = DiskDispatch;
    driverObject->addDevice = DiskAddDevice;
    DiskLoggingInit();
    return OK;
}

