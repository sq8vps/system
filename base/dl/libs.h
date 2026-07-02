#ifndef LIBS_H_
#define LIBS_H_

#include <stddef.h>
#include <stdint.h>
#include "defines.h"

struct Elf32_Ehdr;
struct Elf32_Sym;

struct DlLibrary
{
	struct Elf32_Ehdr *h; /**< SO header = base */
	struct Elf32_Sym *dynSym; /**< Dynamic symbol table */
    size_t dynSymCount; /**< Number of dynamic symbols */
	char *dynStr; /**< Dynamic string table */
    size_t dynStrSize; /**< Size of the dynamic string table */
    struct
    {
        uint32_t nbucket;
        uint32_t nchain;
        uint32_t data[];
    } 
    *hashTab; /**< Hash table */
	bool ready; /**< Is this library already relocated and ready to use? */
	struct DlLibrary *next; /**< Next SO in the list */
	char path[]; /**< SO path */
};

struct DlState
{
	struct 
	{
		size_t count;
		struct DlLibrary *list;
	} loaded;
} ;

extern struct DlState DlState;

/**
 * @brief Load and relocate all libraries required for an ELF object
 * @param *h ELF header of the object
 * @param **envp Environmental variables pointers
 * @return Status code
 */
STATUS DlLoadLibs(const struct Elf32_Ehdr *h, char **envp);

/**
 * @brief Insert ourselves (dynamic loader) to loaded libraries list
 * @param *progHdr Main program ELF header
 * @param *dlHdr ELF header of this dynamic loader
 * @return Status code
 */
STATUS DlInsertLoaderToList(const struct Elf32_Ehdr *progHdr, struct Elf32_Ehdr *dlHdr);

#endif