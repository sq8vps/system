#include "hal.h"
#include "interrupt.h"
#include "time.h"
#include "common.h"
#include "math.h"
#include "mm/heap.h"

static char *HalRootDeviceId = NULL;

STATUS HalInit(void)
{
    STATUS status = OK;

    status = HalInitializeHardware();
    if(OK != status)
        return status;

    status = HalInitMath();
    if(OK != status)
        return status;

    status = HalInitializeRoot();
    if(OK != status)
        return status;
    
    status = HalInitInterruptController();
    if(OK != status)
        return status;

    status = HalInitTimeController();

    return status;
}

char *HalGetRootDeviceId(void)
{
    return HalRootDeviceId;
}

void HalSetRootDeviceId(const char *id)
{
    if(NULL != HalRootDeviceId)
        return;
    HalRootDeviceId = MmAllocateKernelHeap(CmStrlen(id) + 1);
    if(NULL != HalRootDeviceId)
        CmStrcpy(HalRootDeviceId, id);
}