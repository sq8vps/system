#include "write.h"
#include "io/dev/rp.h"
#include "vt.h"
#include "io/dev/dev.h"
#include "device.h"

void TtyWrite(struct IoRp *rp)
{
    if(unlikely(IO_RP_WRITE != rp->code))
        rp->status = BAD_PARAMETER;
    else
    {
        struct TtyDeviceData *info = rp->device->privateData;
        if(TTY_TYPE_VT == info->type)
        {
            if(likely(NULL != rp->payload.write.systemBuffer))
                TtyPutString(info, rp->payload.write.systemBuffer, rp->size);
        }

        rp->status = OK;
    }

    IoFinalizeRp(rp);
}