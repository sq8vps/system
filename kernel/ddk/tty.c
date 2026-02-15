#include "tty.h"
#include "io/dev/dev.h"
#include "io/dev/rp.h"
#include "io/fs/fs.h"
#include "ke/sched/sched.h"
#include "ke/sys/llsyscall.h"
#include "mm/tmem.h"
#include "rtl/string.h"

STATUS TtyCreateVt(struct IoDeviceObject *const dev, struct TtyParameters *const params)
{
    STATUS status = OK;

    if(!dev || !params)
        return BAD_PARAMETER;
    
    if(IO_DEVICE_TYPE_TERMINAL != dev->type)
        return NOT_SUPPORTED;
    
    struct IoRp *rp = IoCreateRp();
    if(NULL == rp)
        return OUT_OF_RESOURCES;
    
    rp->code = IO_RP_TERMINAL_CONTROL;
    rp->payload.deviceControl.code = TTY_CREATE_VT;
    rp->payload.deviceControl.data = params;
    
    status = IoSendRpSync(dev, rp);

    IoFreeRp(rp);
    
    return status;
}

DEFINE_SYSCALL(STATUS, ApiCreateVt, int, int, int, char*);
STATUS ApiCreateVt(int masterHandle, int inputEvent, int outputEvent, char *name)
{
    STATUS status = OK;
    struct IoVfsNode *node =  NULL;
    struct IoDeviceObject *dev = NULL;
    struct TtyParameters params = 
    {
        .request.createVt.inputEvent = inputEvent,
        .request.createVt.outputDisplay = outputEvent,
    };

    if((NULL != name) && !MmProbeUserMemory(name, sizeof(params.request.createVt.name), MM_TASK_MEMORY_READABLE))
        return BAD_PARAMETER;

    //TODO: check permissions

    node = IoGetVfsNodeForFile(KeGetCurrentTaskParent(), masterHandle);
    if(NULL == node)
        return NOT_FOUND;

    status = IoGetDeviceForFile(node, &dev);
    if(OK != status)
        return status;
    if(NULL == dev)
        return NOT_FOUND;

    status = TtyCreateVt(dev, &params);
    if(OK != status)
        return status;

    if(NULL != name)
        RtlStrcpy(name, params.request.createVt.name);

    return OK;
}