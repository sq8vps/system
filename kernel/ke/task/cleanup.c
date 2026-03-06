#include "task.h"
#include "ke/sched/sched.h"
#include "mm/tmem.h"
#include "hal/mm.h"
#include "io/fs/fs.h"
#include "hal/task.h"

STATUS KeDestroyTask(struct KeTaskControlBlock *tcb)
{
    STATUS status = OK;
    struct KeProcessControlBlock *pcb = tcb->parent;
    KeDissociateTCB(tcb);
    if((nullptr != pcb) && (0 == pcb->tasks.count))
    {
        MmFreeAllProcessMemoryOnExit(pcb);
        IoCloseAllFilesOnExit(pcb);
        HalDestroyProcess(pcb);
    }

    HalDestroyTask(tcb);
    return status;
}