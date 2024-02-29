#include "stor.h"
#include "io/dev/rp.h"
#include "mm/heap.h"
#include "assert.h"

// static inline StorReadWrite(struct StorTransfer *descriptor, bool write)
// {
//     ASSERT(descriptor);
//     STATUS status = OK;

//     if(!descriptor->target || !descriptor->completionCallback)
//         return NULL_POINTER_GIVEN;
    
//     struct IoRp *rp = NULL;
//     status = IoCreateRp(&rp);
//     if(OK != status)
//         return status;
    
//     rp->code = IO_RP_STORAGE_CONTROL;
//     rp->payload.deviceControl.code = write ? STOR_WRITE : STOR_READ;
//     rp->completionCallback = descriptor->completionCallback;
//     rp->completionContext = descriptor->completionContext;
//     rp->cancelCallback = descriptor->cancellationCallback;
//     rp->payload.deviceControl.data = descriptor;

//     return IoSendRp(descriptor->target, rp);
// }

// STATUS StorRead(struct StorTransfer *descriptor)
// {
//     StorReadWrite(descriptor, false);
// }

// STATUS StorWrite(struct StorTransfer *descriptor)
// {
//     StorReadWrite(descriptor, true);
// }

STATUS StorGetGeometry(struct IoDeviceObject *target, struct StorGeometry **geometry)
{
    STATUS status = OK;

    if(!target || geometry)
        return NULL_POINTER_GIVEN;
    
    if(IO_DEVICE_TYPE_STORAGE != target->type)
        return IO_DEVICE_INCOMPATIBLE;
    
    struct IoRp *rp = NULL;
    status = IoCreateRp(&rp);
    if(OK != status)
        return status;
    
    rp->code = IO_RP_STORAGE_CONTROL;
    rp->payload.deviceControl.code = STOR_GET_GEOMETRY;
    rp->flags = IO_DRIVER_RP_FLAG_SYNCHRONOUS;
    rp->payload.deviceControl.data = NULL;
    
    status = IoSendRp(target, rp);  

    if(OK == status)
        *geometry = rp->payload.deviceControl.data;
    
    return status;
}
