#include "stor.h"
#include "io/dev/rp.h"
#include "io/dev/dev.h"

STATUS StorGetGeometry(struct IoDeviceObject *target, struct StorGeometry **geometry)
{
    STATUS status = OK;

    if(!target || !geometry)
        return BAD_PARAMETER;
    
    if(IO_DEVICE_TYPE_STORAGE != target->type)
        return BAD_TYPE;
    
    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    rp->code = IO_RP_STORAGE_CONTROL;
    rp->payload.deviceControl.code = STOR_GET_GEOMETRY;
    rp->payload.deviceControl.data = NULL;
    
    status = IoSendRpSync(target, rp);
    if(OK == status)
        *geometry = rp->payload.deviceControl.data;

    IoFreeRp(rp);
    
    return status;
}
