#include <ex/load.h>
#include <ke/task/task.h>
#include <stddef.h>
#include <limits.h>
#include "elf.h"

typedef void (*EntryType)(int, char**, char**, struct ExProgramData*);

[[noreturn]] void _start(int argc, char **argv, char **envp, struct ExProgramData *progData)
{
    EntryType entry = nullptr;
    struct ExProgramData *p = progData;
    struct Elf32_Ehdr *h = nullptr;
    while(PROGDATA_END != p->type)
    {
        if(PROGDATA_BASE == p->type)
        {
            h = p->value.p;
        }
        ++p;
    }

    if(nullptr == h)
        ApiExitTask(-1);

    if((ExVerifyElf32Header(h) < 0) || (ET_DYN != h->e_type) || (0 == h->e_entry))
        ApiExitTask(-1);

    entry = (void*)((uintptr_t)h + h->e_entry);
    entry(argc, argv, envp, progData);

    ApiExitTask(INT_MIN);
    
    while(1)
        ;
}