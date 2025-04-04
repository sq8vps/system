#include "disk.h"
#include "io/dev/rp.h"
#include "io/dev/dev.h"

STATUS DiskGetSignature(struct IoDeviceObject *target, char **signature)
{
    STATUS status = OK;

    if(!target || !signature)
        return NULL_POINTER_GIVEN;
    
    if(IO_DEVICE_TYPE_DISK != target->type)
        return SYSTEM_INCOMPATIBLE;
    
    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    rp->code = IO_RP_DISK_CONTROL;
    rp->payload.deviceControl.code = DISK_GET_SIGNATURE;
    rp->payload.deviceControl.data = NULL;
    
    status = IoSendRpSync(target, rp);
    if(OK == status)
        *signature = rp->payload.deviceControl.data;

    IoFreeRp(rp);
    
    return status;
}
