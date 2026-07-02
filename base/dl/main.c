#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <ex/load.h>
#include <ke/task/task.h>
#include "elf.h"
#include "libs.h"
#include <errno.h>
#include <stdlib.h>

typedef int (*EntryType)(int, char**, char**, struct ExProgramData*);

size_t DlPageSize = 4096;
const char *DlPath = nullptr;

int main(int argc, char **argv, char **envp, struct ExProgramData *progData)
{
    STATUS status = OK;
    struct ExProgramData *p = progData;
    uintptr_t base = 0; //program base
    struct Elf32_Ehdr *h = nullptr; //ELF header of the main file
    struct Elf32_Ehdr *dlHdr = nullptr; //ELF header of the dynamic loader
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
            case PROGDATA_LINKER_BASE:
                dlHdr = p->value.p;
                break;
        }
        ++p;
    }

    status = DlVerifyElf32Header(h);
    if((OK != status) || (ET_DYN != h->e_type) || (0 == h->e_entry))
        exit(-status);

    status = DlInsertLoaderToList(h, dlHdr);
    if(OK != status)
        exit(-status);

    status = DlLoadLibs(h, envp);
    if(OK != status)
        exit(-status);

    status = DlPerformRelocations(h);
    if(OK != status)
        exit(-status);


    entry = (void*)(h->e_entry + base);

    int result = entry(argc, argv, envp, progData);
    //we expect the program to exit on it's own anyway
    exit(result);
}