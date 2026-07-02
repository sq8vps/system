#include "fat.h"
#include "structs.h"
#include "io/dev/op.h"
#include "io/dev/dev.h"
#include "mm/heap.h"
#include "io/fs/vfs.h"
#include "io/dev/rp.h"
#include "utils.h"
#include "ddk/fs.h"
#include "rtl/order.h"
#include "logging.h"
#include "rtl/string.h"

uint32_t FatGetNextCluster(struct FatVolume *vol, uint32_t currentCluster)
{
    uint32_t c = 0;
    switch(vol->type)
    {
        case FAT16:
            c = RtlLeU16(((uint16_t*)vol->fat)[currentCluster]);
            break;
        case FAT32:
            c = RtlLeU32(((uint32_t*)vol->fat)[currentCluster]) & 0xFFFFFFF;
            break;
        case FAT12:
            //TODO: untested for FAT12
            if(currentCluster & 1) //odd cluster number
            {
                c = ((uint8_t*)vol->fat)[((currentCluster - 1) / 2) * 3] >> 4;
                c |= ((uint16_t)((uint8_t*)vol->fat)[((currentCluster - 1) / 2) * 3 + 1] << 4);
                c &= 0xFFF;
            }
            else //even cluster number
            {
                c = ((uint8_t*)vol->fat)[(currentCluster / 2) * 3];
                c |= ((uint16_t)((uint8_t*)vol->fat)[(currentCluster / 2) * 3 + 1] << 8);
                c &= 0xFFF;
            }
            break;
    }
    return c;
}

bool FatIsClusterEof(struct FatVolume *vol, uint32_t cluster)
{
    uint32_t val = FatGetNextCluster(vol, cluster);
    switch(vol->type)
    {
        case FAT16:
            return (RtlLeU16(val) == 0xFFFF);
            break;
        case FAT32:
            return (RtlLeU32(val) == 0xFFFFFFF);
            break;
        case FAT12:
            return (RtlLeU16(val) == 0xFFF);
            break;
    }
    return false;
}

void FatWriteNextCluster(struct FatVolume *vol, uint32_t currentCluster, uint32_t nextCluster)
{
    if(currentCluster < vol->modifiedClusterLow)
        vol->modifiedClusterLow = currentCluster;
    if(currentCluster > vol->modifiedClusterHigh)
        vol->modifiedClusterHigh = currentCluster;
    switch(vol->type)
    {
        case FAT16:
            ((uint16_t*)vol->fat)[currentCluster] = RtlLeU16(nextCluster);
            break;
        case FAT32:
            ((uint32_t*)vol->fat)[currentCluster] &= 0xF0000000;
            ((uint32_t*)vol->fat)[currentCluster] |= (RtlLeU32(nextCluster) & 0xFFFFFFF);
            break;
        case FAT12:
            if(currentCluster & 1) //odd cluster number
            {
                ((uint8_t*)vol->fat)[((currentCluster - 1) / 2) * 3] &= 0x0F;
                ((uint8_t*)vol->fat)[((currentCluster - 1) / 2) * 3] |= ((nextCluster & 0xF) << 4);
                ((uint8_t*)vol->fat)[((currentCluster - 1) / 2) * 3 + 1] = (nextCluster >> 4);
            }
            else //even cluster number
            {
                ((uint8_t*)vol->fat)[(currentCluster / 2) * 3] = nextCluster & 0xFF;
                ((uint8_t*)vol->fat)[(currentCluster / 2) * 3 + 1] &= 0xF0;
                ((uint8_t*)vol->fat)[(currentCluster / 2) * 3 + 1] |= ((nextCluster >> 8) & 0xF);
            }
            break;
    }    
}

uint32_t FatGetConsecutiveClusterCount(struct FatVolume *vol, uint32_t cluster)
{
    uint32_t count = 0;
    uint32_t newCluster = cluster;
    do
    {
        cluster = newCluster;
        newCluster = FatGetNextCluster(vol, cluster);
        if(!FAT_CLUSTER_VALID(vol, newCluster))
            return count + 1;
        count++;
    }
    while(newCluster == (cluster + 1));
    return count;
}

uint32_t FatGetLastCluster(struct FatVolume *vol, uint32_t cluster)
{
    while(!FAT_CLUSTER_VALID(vol, cluster))
    {
        cluster = FatGetNextCluster(vol, cluster);
    }
    return cluster;
}

uint32_t FatReserveClusters(struct FatVolume *vol, uint32_t cluster, uint32_t count)
{
    uint32_t first = cluster;
    uint32_t current = 2;
    while(count && (current < vol->clusters))
    {
        if(FAT_CLUSTER_FREE(FatGetNextCluster(vol, current)))
        {
            if(0 != cluster)
                FatWriteNextCluster(vol, cluster, current);
            FatWriteNextCluster(vol, current, FAT_CLUSTER_EOF);
            if(0 == first)
                first = current;
            cluster = current;
            count--;
        }
        current++;
    }
    return first;
}

void FatFreeClusters(struct FatVolume *vol, uint32_t cluster)
{
    uint32_t c = FatGetNextCluster(vol, cluster);
    FatWriteNextCluster(vol, cluster, FAT_CLUSTER_EOF);
    while(!FatIsClusterEof(vol, c))
    {
        c = FatGetNextCluster(vol, c);
    }
    FatWriteNextCluster(vol, c, 0);
}

static void FatUpdateOnDiskCallback(STATUS status, size_t actualSize, void *context)
{
    if(OK != status)
    {
        LOG(SYSLOG_ERROR, "Writing FAT #%d failed with status 0x%X", (int)context, status);
    }
}

void FatUpdateOnDisk(struct FatVolume *vol)
{
    STATUS status = OK;
    PRIO prio = KeAcquireSpinlock(&(vol->fatLock));
    if(vol->modifiedClusterLow <= vol->modifiedClusterHigh)
    {
        uint64_t lowShift = 0, highShift = 0;
        switch(vol->type)
        {
            case FAT16:
                lowShift = vol->modifiedClusterLow * 2;
                highShift = vol->modifiedClusterHigh * 2;
                break;
            case FAT32:
                lowShift = vol->modifiedClusterLow * 4;
                highShift = vol->modifiedClusterHigh * 4;
                break;
            case FAT12:
                lowShift = (3 * vol->modifiedClusterLow) / 2;
                highShift = (3 * vol->modifiedClusterHigh) / 2;
                break;
        }
        vol->modifiedClusterHigh = 0;
        vol->modifiedClusterLow = UINT32_MAX;
        KeReleaseSpinlock(&(vol->fatLock), prio);
        lowShift = ALIGN_DOWN(lowShift, vol->disk->blockSize);
        highShift = ALIGN_UP(highShift, vol->disk->blockSize);
        for(uint8_t i = 0; i < vol->fatCount; i++)
        {
            status = IoReadWrite(true, vol->disk, NULL, 
                (vol->reservedSectors + i * vol->fatSize) * vol->disk->blockSize + lowShift, highShift - lowShift, 
                (char*)vol->fat + lowShift, FatUpdateOnDiskCallback, (void*)(i + 1), false);
            if(OK != status)
                LOG(SYSLOG_ERROR, "Writing FAT #%d failed with status 0x%X", (int)(i + 1), status);
        }
        return;
    }
    KeReleaseSpinlock(&(vol->fatLock), prio);
}