#include "initrd.h"
#include "common.h"

void Initrd_create(uintptr_t addr)
{
    InitrdHeader_t *h = (InitrdHeader_t*)addr;
    memcpy(h->magic, INITRD_MAGIC_NUMBER, sizeof(INITRD_MAGIC_NUMBER));
    h->firstFileOffset = 0;
}

void Initrd_add(InitrdHeader_t *initrd)
{

}
