#include "kdrv.h"
#include "exec.h"
#include "elf.h"
#include "../mm/heap.h"
#include "../common.h"
#include "../ddk/kdrv_defines.h"
#include <stdbool.h>

#include "../../drivers/vga/vga.h"

#define KDRV_INITIAL_INDEX 0

struct KDrv_DriverListEntry
{
    uintptr_t index;
    DDK_KDrvMetadata_t *meta;
    void (*entry)();
    uintptr_t *vaddr;
    uintptr_t size;
    uint8_t started : 1;

    
    struct KDrv_DriverListEntry *next;
};

struct KDrv_DriverListEntry *kernelModeDriverList = NULL;

struct KDrv_DriverListEntry* ex_getLastKernelModeDriverEntry(void)
{
    struct KDrv_DriverListEntry *t = kernelModeDriverList;
    if(NULL == t)
        return NULL;

    while(NULL != t->next)
    {
        t++;
    }

    return t;
}

kError_t ex_pushDriverMetadata(struct KDrv_DriverListEntry *e)
{
    struct KDrv_DriverListEntry *m = Mm_allocateKernelHeap(sizeof(struct KDrv_DriverListEntry)); //allocate memory for entry
    if(NULL == m)
        return EXEC_MALLOC_FAILED;
    
    Cm_memcpy(m, e, sizeof(*m)); //copy entry

    m->next = NULL; //just to be sure

    if(NULL == kernelModeDriverList) //initialize head if not initialized
    {
        m->index = KDRV_INITIAL_INDEX;
        kernelModeDriverList = m;
    }
    else //head initialized, update last entry
    {
        m->index = ex_getLastKernelModeDriverEntry()->index + 1;
        ex_getLastKernelModeDriverEntry()->next = m;
    }
    
    return OK;
}

kError_t Ex_loadPreloadedDriver(uintptr_t *driverImage, uintptr_t size)
{
    kError_t ret = OK;

    struct Elf32_Ehdr *h = (struct Elf32_Ehdr*)driverImage; //get ELF header

    ret = Ex_verifyElf32Header(h); //verify header
    if(OK != ret)
        return ret;

    if(ET_REL != h->e_type) //only relocatable drivers are supported
        return EXEC_UNSUPPORTED_KERNEL_MODE_DRIVER_TYPE;

    ret = Ex_performElf32Relocation(h, Ex_getKernelSymbol); //perform relocation
    if(OK != ret)
        return ret;

    uintptr_t entry = 0;
    ret = Ex_getElf32SymbolValueByName(h, EXPAND_AND_STRINGIFY(KDRV_ENTRY), &entry); //get entry point location
    if(OK != ret)
        return ret;
    
    uintptr_t meta = 0;
    ret = Ex_getElf32SymbolValueByName(h, EXPAND_AND_STRINGIFY(KDRV_METADATA), &meta); //get metadata location
    if(OK != ret)
        return ret;

    //store driver metadata
    struct KDrv_DriverListEntry m;
    m.vaddr = driverImage;
    m.size = size;
    m.meta = (DDK_KDrvMetadata_t*)meta;
    m.entry = (void(*)())entry;
    m.started = 0;
    m.next = NULL;
    ex_pushDriverMetadata(&m);

    m.entry(); //call driver entry (initialize driver)

    return OK;
}