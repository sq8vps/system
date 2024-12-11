#include "write.h"
#include "io/dev/rp.h"

#include "hal/i686/bootvga/bootvga.h"

void TtyWrite(struct IoRp *rp)
{
    if(unlikely(IO_RP_WRITE != rp->code))
        rp->status = RP_PROCESSING_FAILED;
    else
    {
        BootVgaPrintStringN(rp->payload.write.systemBuffer, rp->size);
        rp->status = OK;
    }

    IoFinalizeRp(rp);
}