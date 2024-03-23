#include "fsdrv.h"
#include "kdrv.h"
#include "io/dev/dev.h"
#include "ke/core/mutex.h"
#include "ob/ob.h"

struct
{
    struct ExDriverObject *list;
    KeSpinlock lock;
} static ExFsDriverState = {.list = NULL, .lock = KeSpinlockInitializer};


STATUS ExMountVolume(struct IoDeviceObject *disk)
{
    STATUS status = OK;

    struct ExDriverObject *drv = NULL;
    KeAcquireSpinlock(&(ExFsDriverState.lock));
    if(NULL != ExFsDriverState.list)
    {
        drv = ExFsDriverState.list;
        KeReleaseSpinlock(&(ExFsDriverState.lock));
        ObLockObject(drv);
        while(drv)
        {
            if(NULL != drv->mount)
            {
                drv->referenceCount++;
                ObUnlockObject(drv);

                status = drv->mount(drv, disk);

                ObLockObject(drv);
                drv->referenceCount--;
                if(OK == status)
                {
                    ObUnlockObject(drv);
                    return OK;
                }
            }
            struct ExDriverObject *next = drv->next;
            ObUnlockObject(drv);
            drv = next;
            ObLockObject(drv);
        }
        ObUnlockObject(drv);
    }
    else
    {
        KeReleaseSpinlock(&(ExFsDriverState.lock));

        status = ExLoadKernelDriver("/initrd/fat.drv", &drv);
        if(OK != status)
            return status;
        
        ObLockObject(drv);
        drv->referenceCount++;
    }
}