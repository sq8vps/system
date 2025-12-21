#include "write.h"
#include "io/dev/rp.h"
#include "vt.h"
#include "io/dev/dev.h"
#include "device.h"

void TtyRead(struct IoRp *rp)
{
    struct TtyDeviceData *info = rp->device->privateData;
    if(0 == rp->size)
    {
        rp->status = OK;
        IoFinalizeRp(rp);
        return;
    }

    if(NULL == rp->payload.read.systemBuffer)
    {
        rp->status = BAD_PARAMETER;
        IoFinalizeRp(rp);
        return;
    }

    if((TTY_TYPE_DUMMY == info->type) 
        || ((TTY_TYPE_VT == info->type) && (info->vt.input.handle < 0)))
    {
        rp->status = NOT_SUPPORTED;
        IoFinalizeRp(rp);
        return;
    }
    
    PRIO prio = KeAcquireDpcLevelSpinlock(&info->input.lock);

    if(0 != info->input.lines)
    {
        size_t s = (rp->size > RingBufferGetSize(&info->input.ring)) ? 
            RingBufferGetSize(&info->input.ring) : rp->size;

        if(likely(0 != s))
        {
            for(size_t k = 0; k < s; k++)
            {
                char c = RingBufferPop(&info->input.ring, info->input.buffer);
                ((char*)rp->payload.read.systemBuffer)[k] = c;
                if('\n' == c) //read at most one line
                {
                    --info->input.lines;
                    s = k + 1;
                    break;
                }
            }
        }
        rp->size = s;
        rp->status = OK;
        KeReleaseSpinlock(&info->input.lock, prio);
        IoFinalizeRp(rp);
    }
    else
    {
        info->input.currentRp = rp;
        KeReleaseSpinlock(&info->input.lock, prio);
    }
}