#include "disk.h"
#include "mm/mm.h"

STATUS DiskReadWrite(struct IoRp *rp)
{
    struct IoDeviceObject *dev = IoGetCurrentRpPosition(rp);
    struct DiskData *info = dev->privateData;
    if(info->isMdo && info->isPartition0)
    {
        //we are the MDO of partition 0
        //assume this request is already prepared
        //a) by the kernel if a flat disk operation is requested
        //b) by the higher level driver when a partition operation is requested
        //just forward the request to the drive controller
        return IoSendRpDown(rp);
    }
    else if(!info->isMdo && !info->isPartition0)
    {
        //we are the BDO of partition n
        //add appropriate offsets to work on given partition
        //also check if the request fits in the partition
        if(IO_RP_READ == rp->code)
        {
            uint64_t size;
            if(dev->flags & IO_DEVICE_FLAG_DIRECT_IO)
            {
                size = MmGetMemoryDescriptorListSize(rp->payload.read.memory);
            }
            else if(dev->flags & IO_DEVICE_FLAG_BUFFERED_IO)
            {
                size = rp->size;
            }
            else
            {
                //TODO: implement user mode buffer handling
                rp->status = NOT_COMPATIBLE;
                IoFinalizeRp(rp);
                return OK;
            }

            if(size > info->partition.sizeBytes)
            {
                //won't fit
                rp->status = IO_VOLUME_TOO_SMALL;
                IoFinalizeRp(rp);
                return OK;
            }
            rp->payload.read.offset += info->partition.startBytes;
        }
        else if(IO_RP_WRITE == rp->code)
        {
            uint64_t size;
            if(dev->flags & IO_DEVICE_FLAG_DIRECT_IO)
            {
                size = MmGetMemoryDescriptorListSize(rp->payload.write.memory);
            }
            else if(dev->flags & IO_DEVICE_FLAG_BUFFERED_IO)
            {
                size = rp->size;
            }
            else
            {
                //TODO: implement user mode buffer handling
                rp->status = NOT_COMPATIBLE;
                IoFinalizeRp(rp);
                return OK;
            }

            if(size > info->partition.sizeBytes)
            {
                //won't fit
                rp->status = IO_VOLUME_TOO_SMALL;
                IoFinalizeRp(rp);
                return OK;
            }
            rp->payload.write.offset += info->partition.startBytes;
        }
        else
        {
            rp->status = IO_RP_PROCESSING_FAILED;
            IoFinalizeRp(rp);
            return OK;
        }
        
        //everything is fine, forward RP
        return IoSendRp(info->part0device, rp);
    }
    else
    {
        //bad disk configuration, should not happen
        rp->status = IO_RP_PROCESSING_FAILED;
        IoFinalizeRp(rp);
        return OK;
    }
}