#include "stor.h"
#include "io/dev/rp.h"
#include "mm/heap.h"
#include "assert.h"

STATUS StorRead(struct StorTransfer *descriptor)
{
    ASSERT(descriptor);
    STATUS status = OK;

    if(!descriptor->target || !descriptor->completionCallback)
        return NULL_POINTER_GIVEN;
    
    struct IoRp *rp = NULL;
    status = IoCreateRp(&rp);
    if(OK != status)
        return status;
    
    rp->code = IO_RP_STORAGE_CONTROL;
    rp->payload.deviceControl.code = STOR_READ;
    rp->completionCallback = descriptor->completionCallback;
    rp->completionContext = descriptor->completionContext;
    rp->cancelCallback = descriptor->cancellationCallback;
    rp->payload.deviceControl.data = descriptor;

    return IoSendRp(descriptor->target, rp);
}

