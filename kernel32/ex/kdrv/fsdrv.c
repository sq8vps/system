#include "fsdrv.h"
#include "kdrv.h"
#include "io/dev/dev.h"
#include "ke/core/mutex.h"
#include "ob/ob.h"

struct
{
    struct ExDriverObject *list;
    KeMutex mutex;
} static ExFsDriverState = {.list = NULL, .mutex = KeMutexInitializer};


STATUS ExMountVolume(struct IoDeviceObject *disk)
{
    STATUS status = OK;

    struct ExDriverObject *drv = NULL;
    KeAcquireMutex(&(ExFsDriverState.mutex));
    if(NULL != ExFsDriverState.list)
    {
        drv = ExFsDriverState.list;
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
                    KeReleaseMutex(&(ExFsDriverState.mutex));
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
        status = ExLoadKernelDriver("/initrd/fat.drv", &drv);
        if(OK != status)
        {
            KeReleaseMutex(&(ExFsDriverState.mutex));
            return status;
        }
        
        ObLockObject(drv);
        drv->referenceCount++;
        ObUnlockObject(drv);

        status = drv->mount(drv, disk);

        ObLockObject(drv);
        drv->referenceCount--;
        if(OK == status)
        {
            ExFsDriverState.list = drv;
        }
        ObUnlockObject(drv);
    }

    KeReleaseMutex(&(ExFsDriverState.mutex));
    return status;
}