#include "disk.h"
#include "mm/mm.h"
#include "rtl/uuid.h"
#include "rtl/string.h"
#include "rtl/stdio.h"

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

STATUS DiskGetSig(struct DiskData *info, char **signature)
{
    char *t = MmAllocateKernelHeap(RTL_UUID_STRING_LENGTH + 64);
    if(NULL == t)
        return OUT_OF_RESOURCES;
    
    RtlStrcpy(t, "DISK:SIG={");


    if(!info->isMdo) //if it is a BDO, and since there are no BDOs for partition 0, this must be a partition x
    {
        //get partition 0 UUID
        struct DiskData *part0info = info->part0device->privateData;
        if(part0info->isGpt)
        {

        }
        else if(part0info->isMbr)
            DiskMbrGetUuid(part0info, t + RtlStrlen(t));
        //no need to check for non-partitioned disks, since we would never end here
    }
    else //if this is an MDO, this must be partition 0
    {
        if(info->isGpt)
        {

        }
        else if(info->isMbr)
            DiskMbrGetUuid(info, t + RtlStrlen(t));
        else
        {
            //fail on non-partitioned disks, since there is no obtainable UUID
            MmFreeKernelHeap(t);
            return IO_RP_PROCESSING_FAILED;
        }
    }

    if(info->isMdo) //MDO of a partition 0
    {
        RtlStrcat(t, "}&PAR=0");
    }
    else //BDO of partition x
    {
        sprintf(t + RtlStrlen(t), "}&PAR=%lu", info->partition.number + 1);
    }

    *signature = t;

    return OK;
}