#include "templates/id.hpp"

#define KE_MAX_TASK_ID 65535

static IdDispenser<uint16_t, KE_MAX_TASK_ID> KeTaskIdDispenser; 

extern "C"
{
uint16_t KeAssignTid(void)
{
    return KeTaskIdDispenser.assign();
}

void KeFreeTid(uint16_t tid)
{
    KeTaskIdDispenser.free(tid);
}
}