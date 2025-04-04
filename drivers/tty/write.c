#include "write.h"
#include "io/dev/rp.h"


void TtyWrite(struct IoRp *rp)
{
    if(unlikely(IO_RP_WRITE != rp->code))
        rp->status = RP_PROCESSING_FAILED;
    else
    {
        
        rp->status = OK;
    }

    IoFinalizeRp(rp);
}