#include "fsdrv.h"
#include "kdrv.h"
#include "io/dev/dev.h"
#include "ke/core/mutex.h"
#include "ob/ob.h"
#include "io/dev/vol.h"
#include "mm/heap.h"

STATUS ExMountVolume(struct IoVolumeNode *volume)
{
    STATUS status = OK;
    struct ExDriverObjectList *drivers = NULL;
    uint16_t driverCount = 0;

    status = ExLoadKernelDriversForFilesystem(volume, &drivers, &driverCount);
    if(OK != status)
        return status;
    if((0 == driverCount) || (NULL == drivers))
        return status;

    struct ExDriverObjectList *d = drivers;
    //first driver is assumed to be the main driver
    if(NULL == d->this->mount)
    {
        status = DEVICE_NOT_AVAILABLE;
        goto ExMountVolumeFailed;
    }

    status = d->this->mount(d->this, volume->pdo);
    if(OK != status)
        goto ExMountVolumeFailed;

    for(uint16_t i = 1; i < driverCount; i++)
    {
        if(NULL == d)
            goto ExMountVolumeFailed;
        status = d->this->addDevice(d->this, volume->fsdo);
        if(OK != status)
            goto ExMountVolumeFailed;
        d = d->next;
    }

ExMountVolumeFailed:
    
    while(NULL != drivers)
    {
        d = drivers->next;
        MmFreeKernelHeap(drivers);
        drivers = d;
    }

    return status;
}