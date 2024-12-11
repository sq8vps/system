#include "stor.h"
#include "io/dev/rp.h"
#include "io/dev/dev.h"

STATUS StorGetGeometry(struct IoDeviceObject *target, struct StorGeometry **geometry)
{
    STATUS status = OK;

    if(!target || !geometry)
        return NULL_POINTER_GIVEN;
    
    if(IO_DEVICE_TYPE_STORAGE != target->type)
        return SYSTEM_INCOMPATIBLE;
    
    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    rp->code = IO_RP_STORAGE_CONTROL;
    rp->payload.deviceControl.code = STOR_GET_GEOMETRY;
    rp->payload.deviceControl.data = NULL;
    
    status = IoSendRp(target, rp);
    if(OK == status)
    {
        IoWaitForRpCompletion(rp);
        status = rp->status;
        if(OK == status)
            *geometry = rp->payload.deviceControl.data;
    }

    IoFreeRp(rp);
    
    return status;
}
