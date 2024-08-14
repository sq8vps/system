#include "hal.h"
#include "interrupt.h"
#include "time.h"
#include "common.h"
#include "math.h"
#include "mm/heap.h"

static char *HalRootDeviceId = NULL;

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