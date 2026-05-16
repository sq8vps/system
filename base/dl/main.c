#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <ex/load.h>
#include <ke/task/task.h>
#include "elf.h"
#include <errno.h>

typedef int (*EntryType)(int, char**, char**, struct ExProgramData*);

size_t DlPageSize = 4096;

int main(int argc, char **argv, char **envp, struct ExProgramData *progData)
{
    struct ExProgramData *p = progData;
    uintptr_t base = 0; //program base
    struct Elf32_Ehdr *h = nullptr; //ELF header
    struct Elf32_Phdr *phdr = nullptr; //program data headers
    struct Elf32_Dyn *dyn = nullptr; //dynamic entries
    struct Elf32_Rel *rel = nullptr; //relocation entries
    size_t relCount = 0; //number of relocation entries
    bool addend = false; //are relocations with addends?
    struct Elf32_Sym *sym = nullptr; //symbol table
    EntryType entry = nullptr;
    while(PROGDATA_END != p->type)
    {
        switch(p->type)
        {
            case PROGDATA_BASE:
                base = (uintptr_t)p->value.p;
                h = p->value.p;
                break;
            case PROGDATA_PAGE_SIZE:
                DlPageSize = p->value.s;
                break;
        }
        ++p;
    }

    if((DlVerifyElf32Header(h) < 0) || (ET_DYN != h->e_type) || (0 == h->e_entry))
    {
        ApiExitTask(-ENOEXEC);
    }



    entry = (void*)(h->e_entry + base);

    int result = entry(argc, argv, envp, progData);
    //we expect the program to exit on it's own anyway
    ApiExitTask(result);
}