#include "templates/id.hpp"
#include "task.h"

#define KE_MAX_TASK_ID 65535

static IdDispenser<KE_TASK_ID, KE_MAX_TASK_ID> KeTaskIdDispenser; 

extern "C"
{
KE_TASK_ID KeAssignTid(void)
{
    return KeTaskIdDispenser.assign();
}

void KeFreeTid(KE_TASK_ID tid)
{
    KeTaskIdDispenser.free(tid);
}
}