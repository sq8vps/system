#include "bridge.h"
#include "kernel.h"

//max number of PCI-PCI bridge levels
//level = 1 means there is only a host bridge
//level = 2 means there is a host bridge and PCI-PCI bridges connected directly to host bridge 
#define BRIDGE_MAX_LEVEL 4

struct PciBridge *PciBridgeList;
static KeSpinlock PciBridgeListMutex = KeSpinlockInitializer;

STATUS PciRegisterBridge(struct PciAddress address, struct PciBridge *upstreamBridge, struct PciBridge **out)
{
    return NOT_IMPLEMENTED;
    // KeAcquireSpinlock(&PciBridgeListMutex);
    // if(NULL == PciBridgeList)
    // {
    //     KeReleaseSpinlock(&PciBridgeListMutex);
    //     return DEVICE_NOT_AVAILABLE;
    // }
    // KeReleaseSpinlock(&PciBridgeListMutex);

    // struct PciBridge *b = MmAllocateKernelHeap(sizeof(*b));
    // if(NULL == b)
    //     return OUT_OF_RESOURCES;

    // b->address = address;
    // b->parent = upstreamBridge;
    // b->next = NULL;
    // b->child = NULL;

    // KeAcquireSpinlock(&PciBridgeListMutex);
    //     // if(NULL == upstreamBridge)
    //     // {
    //     //     KeReleaseSpinlock(&PciBridgeListMutex);
    //     //     MmFreeKernelHeap(b);
    //     //     return NULL_POINTER_GIVEN;
    //     // }
    //     // //TODO: basically untested
    //     // if(upstreamBridge->level >= BRIDGE_MAX_LEVEL)
    //     // {
    //     //     KeReleaseSpinlock(&PciBridgeListMutex);
    //     //     MmFreeKernelHeap(b);
    //     //     return OUT_OF_RESOURCES;
    //     // }

    //     // b->level = upstreamBridge->level + 1;
        
    //     // struct PciBridge *t = upstreamBridge->child;
    //     // while(t->next)
    //     //     last = last->next;
        
    //     // last->next = b;

    // KeReleaseSpinlock(&PciBridgeListMutex);
    // *out = b;
    // return OK;
}

STATUS PciRegisterHostBridge(struct PciAddress address, struct PciBridge **out)
{
    struct PciBridge *b = MmAllocateKernelHeap(sizeof(*b));
    if(NULL == b)
        return OUT_OF_RESOURCES;

    CmMemset(b, 0, sizeof(*b));

    KeAcquireSpinlock(&PciBridgeListMutex);
    //allow only one host bridge
    if(NULL == PciBridgeList)
    {
        PciBridgeList = b;
        KeReleaseSpinlock(&PciBridgeListMutex);
        b->primaryBus = 0;
        b->secondaryBus = 0;
        b->subordinateBus = 255;
        b->level = 0;
        b->nextChildBus = 1;
    }
    else
    {
        KeReleaseSpinlock(&PciBridgeListMutex);
        MmFreeKernelHeap(b);
        return OUT_OF_RESOURCES;
    }
    
    *out = b;
    return OK;
}