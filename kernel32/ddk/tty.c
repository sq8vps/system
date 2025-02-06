#include "tty.h"
#include "io/dev/dev.h"
#include "io/dev/rp.h"

STATUS TtyCreateVt(struct IoDeviceObject *const dev, struct TtyParameters *const params)
{
    STATUS status = OK;

    if(!dev || !params)
        return NULL_POINTER_GIVEN;
    
    if(IO_DEVICE_TYPE_TERMINAL != dev->type)
        return SYSTEM_INCOMPATIBLE;
    
    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    rp->code = IO_RP_TERMINAL_CONTROL;
    rp->payload.deviceControl.code = TTY_CREATE_VT;
    rp->payload.deviceControl.data = params;
    
    status = IoSendRp(dev, rp);
    if(OK == status)
    {
        IoWaitForRpCompletion(rp);
        status = rp->status;
    }

    IoFreeRp(rp);
    
    return status;
}