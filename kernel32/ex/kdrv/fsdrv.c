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
        
        while(drv)
        {
            if(NULL != drv->mount)
            {
                drv->referenceCount++;
                

                status = drv->mount(drv, disk);

                
                drv->referenceCount--;
                if(OK == status)
                {
                    
                    KeReleaseMutex(&(ExFsDriverState.mutex));
                    return OK;
                }
            }
            struct ExDriverObject *next = drv->next;
            
            drv = next;
            
        }
        
    }
    else
    {
        status = ExLoadKernelDriver("/initrd/fat.drv", &drv);
        if(OK != status)
        {
            KeReleaseMutex(&(ExFsDriverState.mutex));
            return status;
        }
        
        
        drv->referenceCount++;
        

        status = drv->mount(drv, disk);

        
        drv->referenceCount--;
        if(OK == status)
        {
            ExFsDriverState.list = drv;
        }
        
    }

    KeReleaseMutex(&(ExFsDriverState.mutex));
    return status;
}