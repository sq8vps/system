#include "task.h"
#include "assert.h"

#define KE_MAX_TASK_ID 65535
static uint16_t nextSequentialTid = 1;
static uint16_t freeTid[65535] = {0}; //a stack of freed task IDs. 0th element is reserved
static uint16_t freeTidTop = 0;
static KeSpinlock tidAssignerMutex = KeSpinlockInitializer;

uint16_t KeAssignTid(void)
{
    uint16_t tid = 0;
    KeAcquireSpinlock(&tidAssignerMutex);
    if(nextSequentialTid < KE_MAX_TASK_ID)
    {
        tid = nextSequentialTid++;
    }
    else if(freeTidTop > 0)
    {
        tid = freeTid[freeTidTop--];
    }
    KeReleaseSpinlock(&tidAssignerMutex);
    return tid;
}

void KeFreeTid(uint16_t tid)
{
    KeAcquireSpinlock(&tidAssignerMutex);
    if(freeTidTop < KE_MAX_TASK_ID)
    {
        freeTid[++freeTidTop] = tid;
    }
    KeReleaseSpinlock(&tidAssignerMutex);
}