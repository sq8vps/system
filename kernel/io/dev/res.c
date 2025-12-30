#include "res.h"
#include "mm/heap.h"
#include "rtl/string.h"

struct IoIrqMap* IoCopyIrqMap(struct IoIrqMap *map)
{
    if(NULL == map)
        return NULL;

    struct IoIrqMap *r = NULL;
    r = MmAllocateKernelHeapZeroed(sizeof(*r));
    if(NULL == r)
        return NULL;
    
    r->type = map->type;
    r->id = map->id;

    if(0 != map->irqCount)
    {
        r->irq = MmAllocateKernelHeap(sizeof(*(r->irq)) * map->irqCount);
        if(NULL == r->irq)
            return NULL;
        RtlMemcpy(r->irq, map->irq, sizeof(*(r->irq)) * map->irqCount);
        r->irqCount = map->irqCount;
    }
    
    r->child = IoCopyIrqMap(map->child);
    if(NULL != r->child)
    {
        struct IoIrqMap *originalChild = map->child->next;
        struct IoIrqMap *newChild = r->child;
        while(NULL != originalChild)
        {
            newChild->next = IoCopyIrqMap(originalChild);
            if(NULL == newChild->next)
                break;
            originalChild = originalChild->next;
            newChild = newChild->next;
        }
    }

    return r;
}