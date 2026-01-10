#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "ndb.h"
#include <unistd.h>
#include <stdarg.h>
#include <ctype.h>

#define VERSION_STRING "Nabla Configuration Database Editor v.0.0.1\n" "This program is licensed under GNU GPL v3.0\n"

#define STRINGIFY(a) #a
#define EXPAND_AND_STRINGIFY(a) STRINGIFY(a)
#define FILE_LINE_STR __FILE__ ":" EXPAND_AND_STRINGIFY(__LINE__)

#define EXIT_ON_NON_INTERACTIVE(code) do{if(!interactive) NdbEditExit((code));} while(0);

static char line[2048]; /**< Input line buffer */
static char *path = NULL; /**< Pointer to path argument */
static char *name = NULL; /**< Pointer to entry name argument */
static char *value = NULL; /**< Pointer to value argument */
static char *typeName = NULL; /**< Pointer to type argument */
static char *newName = NULL; /**< Pointer to new name argument */
static char *indexa = NULL; /**< Pointer to index argument */
static FILE *f = NULL; /**< Database file handle */
static struct NablaDbHeader *h = NULL; /**< Database header and body handle */

static const char helpPage[] =
"Usage: ndbedit [-f file] [-n name] [-t type] [-v value] [-i index] [-x new_name] [-r] [-h] [command]\n"
"\t-f file - database path\n"
"\t-n name - entry or array name\n"
"\t-t type - entry type (required when creating entries or arrays):\n"
"\t\tAllowed types are: byte, word, dword, qword, bool, utf8, timestamp, uuid, float, double, multi\n"
"\t-d value - entry value (required when creating or modyfing entries)\n"
"\t-i index - array index (required when removing or modyfing array elements)\n"
"\t-x new_name - new name for the entry (required when changing entry name)\n"
"\t-r - force database creation (overwrite if already exists, create if does not exist)\n"
"\t-h - show help page and exit\n"
"\t-v - show version info and exit\n"
"\tcommand - command to execute:\n"
"\t\tnew - create new database (required: [-f file], optional: [-r])\n"
"\t\tadd - add new entry (required: [-f file], [-n name], [-t type], [-d value], [-i index] (when adding to an array))\n"
"\t\tvalue - change value (required: [-f file], [-n name], [-d value], [-i index] (when entry is a part of an array))\n"
"\t\tname - change name (required: [-f file], [-n name], [-x new_name]\n"
"\t\tarray - create array (required: [-f file], [-n name]\n"
"\t\tremove - remove entry or array (required: [-f file], [-n name], [-i index] (when removing an element from an array))\n"
"\t\t<none> - run in interactive mode (the other parameters are discared)\n"
"ndbedit can work in standalone or interactive mode.\n"
"Interactive (console) mode is selected whenever no command is specified.\n"
"Provided parameters are not used in interactive mode.\n";

static void writeDb(void)
{
    if(fwrite(h, 1, sizeof(*h) + h->size, f) < (sizeof(*h) + h->size))
    {
        printf("Failed to write database\n");
    }
    else
    {
        printf("Database written succesfully\n");
    }
    fflush(f);
    rewind(f);
}

static void NdbEditExit(int code)
{
    free(h);
    if(NULL != f)
        fclose(f);
    if(0 == code)
        printf("Operation successful\n");
    else
        printf("Operation FAILED\n");
    exit(code);
}

static void fail(const char *string, ...)
{
    va_list args;
    va_start(args, string);
    vprintf(string, args);
    va_end(args);
    NdbEditExit(-1);
}

static bool get(void)
{
    if(NULL == fgets(line, sizeof(line), stdin))
        return false;
    line[strcspn(line, "\r\n")] = '\0';
    return true;
}

static bool GetYesNo(bool defaultOption)
{
    if(!get())
        return defaultOption;
    
    char c;
    sscanf(line, "%c", &c);
    if(('Y' == c) || ('y' == c))
        return true;
    else if(('N' == c) || ('n' == c))
        return false;
    else
        return defaultOption;
}

enum NablaDbType parseType(const char *s)
{
    if(!strcmp(s, "byte"))
        return NDB_BYTE;
    else if(!strcmp(s, "bool"))
        return NDB_BOOL;
    else if(!strcmp(s, "word"))
        return NDB_WORD;
    else if(!strcmp(s, "dword"))
        return NDB_DWORD;
    else if(!strcmp(s, "qword"))
        return NDB_QWORD;
    else if(!strcmp(s, "null"))
        return NDB_NULL;
    else if(!strcmp(s, "utf8"))
        return NDB_UTF8;
    else if(!strcmp(s, "timestamp"))
        return NDB_TIMESTAMP;
    else if(!strcmp(s, "uuid"))
        return NDB_UUID;
    else if(!strcmp(s, "float"))
        return NDB_FLOAT;
    else if(!strcmp(s, "double"))
        return NDB_DOUBLE;
    else if(!strcmp(s, "multi"))
        return NDB_MULTI;
    else
        return NDB_UNKNOWN;
}

static void printElement(struct NablaDbEntry *e)
{
    union NablaDbData *v = NULL;
    struct NablaDbArrayElement *ae = (struct NablaDbArrayElement*)e;
    bool isArrayElement = false;

    if(!NABLADB_IS_ARRAY_ELEMENT(e->type))
    {
        v = (union NablaDbData*)((uintptr_t)(e + 1) + e->nameLength);
        printf("%*s, ", e->nameLength, e->name);
        isArrayElement = false;
    }
    else
    {
        v = (union NablaDbData*)(ae + 1);
        isArrayElement = true;
    }

    if(NDB_ARRAY == e->type)
    {
        if(0 == e->elementCount)
            printf("empty array\n");
        else
            printf("array with %lu entries:\n", e->elementCount);
    }
    else
    {
        switch(NABLADB_TYPE_MASK(e->type))
        {
            case NDB_NULL:
                printf("null");
                break;
            case NDB_BYTE:
                printf("byte, 0x%X", (uint32_t)v->byte);
                break;
            case NDB_BOOL:
                printf("boolean, %s", v->boolean ? "true" : "false");
                break;
            case NDB_WORD:
                printf("word, 0x%X", (uint32_t)v->word);
                break;
            case NDB_DWORD:
                printf("double word, 0x%X", v->dword);
                break;
            case NDB_QWORD:
                printf("quad word, 0x%llX", v->qword);
                break;
            case NDB_FLOAT:
                printf("float, %f", v->fl);
                break;
            case NDB_TIMESTAMP:
                printf("timestamp, %llu", v->timestamp);
                break;
            case NDB_DOUBLE:
                printf("double, %d", v->db);
                break;
            case NDB_UUID:
                printf("UUID, ");
                for(uint8_t i = 0; i < 16; ++i)
                {
                    printf("%X", (uint32_t)(((uint8_t*)v)[i]));
                    if((3 == i) || (5 == i) || (7 == i) || (9 == i))
                        printf("-");
                }
                break;
            case NDB_UTF8:
                printf("string, %*s", isArrayElement ? ae->dataLength : e->dataLength, (char*)v);
                break;
            case NDB_MULTI:
                printf("multiple bytes,");
                for(uint32_t i = 0; i < (isArrayElement ? ae->dataLength : e->dataLength); ++i)
                    printf(" %u", ((uint8_t*)v)[i]);
                break;
            default:
                printf("unknown");
                break;
        }
        printf(" (%u bytes)\n", isArrayElement ? ae->dataLength : e->dataLength);
    }
}

static void list(struct NablaDbHeader *h)
{
    uint32_t count = 0;
    uint32_t element = 0;
    struct NablaDbEntry *e = NablaDbGetEntry(h, NULL);
    while(NULL != e)
    {
        if(NABLADB_IS_ARRAY_ELEMENT(e->type))
        {
            printf("\t%lu (%lu): ", element, count + 1);
            ++element;
        }
        else
        {
            printf("%lu: ", count + 1);
        }
        
        printElement(e);
        
        ++count;
        e = NablaDbGetEntry(h, e);
    }

    if(0 == count)
        printf("Database is empty\n");
}

static struct NablaDbArrayElement *getArrayEntry(bool interactive, const char *index, struct NablaDbHeader *h, struct NablaDbEntry *array)
{   
    uint32_t count = 0;
    struct NablaDbEntry *t = NablaDbGetEntry(h, array);
    while((NULL != t) && (NABLADB_IS_ARRAY_ELEMENT(t->type)))
    {
        if(interactive)
        {
            printf("%lu: ", count);
            printElement(t);
        }
        t = NablaDbGetEntry(h, t);
        ++count;
    }

    if(0 != count)
    {
        uint32_t s = 0;
        if(interactive)
        {
            printf("Select element: ");
            if(!get())
                return NULL;

            sscanf(line, "%lu", &s);
        }
        else
        {
            if(NULL == index)
                fail("Array index must be provided\n");
            
            const char *i = index;
            while('\0' != *i)
            {
                if(!isdigit(*i))
                    fail("Bad array index %s\n", index);
                ++i;
            }

            s = atoi(index);
        }

        if(s >= count)
            return NULL;
        
        struct NablaDbArrayElement *t = (struct NablaDbArrayElement*)NablaDbGetEntry(h, array);
        while((NULL != t) && (NABLADB_IS_ARRAY_ELEMENT(t->type)) && (0 != s))
        {
            --s;
            t = (struct NablaDbArrayElement*)NablaDbGetEntry(h, (struct NablaDbEntry*)t);
        }

        if(0 != s)
            return NULL;
        
        return t;
    }

    return NULL;
}

static bool getValue(const char *line, enum NablaDbType type, uint8_t **v, struct NablaDbEntry **e, uint32_t *length, bool standalone)
{
    const char *t = NULL;
    struct NablaDbArrayElement *ae = NULL;
    if(NULL != e)
        ae = (struct NablaDbArrayElement*)*e;
    bool isArrayElement = (NULL != e) ? NABLADB_IS_ARRAY_ELEMENT((*e)->type) : false;
    
    switch(NABLADB_TYPE_MASK(type))
    {
        case NDB_BYTE:
            **((uint8_t**)v) = strtol(line, NULL, 0);
            break;
        case NDB_WORD:
            **((uint16_t**)v) = strtol(line, NULL, 0);
            break;
        case NDB_DWORD:
            **((uint32_t**)v) = strtol(line, NULL, 0);
            break;
        case NDB_QWORD:
        case NDB_TIMESTAMP:
            **((uint64_t**)v) = strtoll(line, NULL, 0);
            break;
        case NDB_FLOAT:
            **((float**)v) = strtof(line, NULL);
            break;
        case NDB_DOUBLE:
            **((double**)v) = strtod(line, NULL);
            break;
        case NDB_BOOL:
            if(!strcmp(line, "true") || !strcmp(line, "TRUE") || !strcmp(line, "1"))
                **((bool**)v) = true;
            else
                **((bool**)v) = false;
            break;
        case NDB_UUID:
            memset(*v, 0, 16);
            t = line;
            uint32_t i = 0;
            while('\0' != *t)
            {
                uint8_t n = 0;
                if((*t >= '0') && (*t <= '9'))
                    n = *t - '0';
                else if((*t >= 'a') && (*t <= 'f'))
                    n = *t - 'a' + 10;
                else if((*t >= 'A') && (*t <= 'F'))
                    n = *t - 'A' + 10;
                else if(*t == '-')
                {
                    ++t;
                    continue;
                }
                else
                {
                    printf("Bad character \"%c\" in UUID\n", *t);
                    free(*e);
                    continue;
                }

                if(i & 1)
                    (*v)[i >> 1] |= n & 0xF;
                else
                    (*v)[i >> 1] |= (n << 4);

                ++i;
                ++t;
            }
            break;
        case NDB_UTF8:
            if(!standalone)
            {
                if(!isArrayElement)
                {
                    (*e)->dataLength = strlen(line) + 1;
                    *e = realloc(*e, sizeof(**e) + (*e)->nameLength + (*e)->dataLength);
                    
                }
                else
                {
                    ae->dataLength = strlen(line) + 1;
                    ae = realloc(ae, sizeof(*ae) + ae->dataLength);
                    *e = (struct NablaDbEntry*)ae;
                }

                if(NULL == *e)
                {
                    printf("Memory allocation failed\n");
                    return false;
                }

                if(!isArrayElement)
                    strcpy((void*)((uintptr_t)(*e + 1) + (*e)->nameLength), line);
                else
                    strcpy((void*)(ae + 1), line);
            }
            else
            {
                *length = strlen(line) + 1;
                *v = realloc(*v, strlen(line) + 1);
                if(NULL == *v)
                {
                    printf("Memory allocation failed\n");
                    return false;
                }
                strcpy(*v, line);
            }

            break;
        case NDB_MULTI:
            t = line;
            uint32_t elements = 0;
            while('\0' != *t)
            {
                if(' ' == *t)
                    ++elements;
                ++t;
                if('\0' == *t)
                    ++elements;
            }

            if(!standalone)
            {
                if(!isArrayElement)
                    *e = realloc(*e, sizeof(**e) + (*e)->nameLength + elements);
                else
                {
                    ae = realloc(ae, sizeof(*ae) + elements);
                    *e = (struct NablaDbEntry*)ae;
                }

                if(NULL == *e)
                {
                    printf("Memory allocation failed\n");
                    return false;
                }

                if(!isArrayElement)
                    (*e)->dataLength = elements;
                else
                    ae->dataLength = elements;
            }
            else
            {
                *v = realloc(*v, elements);
                *length = elements;

                if(*v == NULL)
                {
                    printf("Memory allocation failed\n");
                    return false;
                }
            }

            t = line;
            for(uint32_t i = 0; i < elements; ++i)
            {
                (*v)[i] = strtoll(t, (char**)&t, 0);
                if('\0' == *t)
                    break;
                ++t;
            }
            break;
    }

    if(standalone)
    {
        if(!NablaDbIsVariableLength(type))
            *length = NablaDbGetTypeLength(type);
    }

    return true;
}

static void newDb(const char *path, struct NablaDbHeader **h)
{
    f = fopen(path, "wb+");
    if(NULL == f)
        fail("Failed to create file %s\n", path);

    if(!NablaDbWriteEmpty(h))
    {
        fail("Failed to write basic database structure\n");
    }

    if(fwrite(*h, 1, (*h)->size + sizeof(**h), f) < ((*h)->size + sizeof(**h)))
    {
        fail("Failed to write basic database structure\n");
    }
    rewind(f);
    printf("Created file %s\n", path);
}

static void readDb(FILE *f, struct NablaDbHeader **h)
{
    *h = malloc(sizeof(**h));
    if(NULL == *h)
        fail("Memory allocation failed\n");

    if(fread(*h, 1, sizeof(**h), f) < sizeof(**h))
        fail("File is not a valid Nabla database\n");
    rewind(f);
    
    *h = realloc(*h, (*h)->size + sizeof(**h));
    if(NULL == *h)
        fail("Memory allocation failed\n");
    
    if(fread(*h, 1, (*h)->size + sizeof(**h), f) < 0)
        fail("Database is broken\n");
    rewind(f);
}

int main(int argc, char **argv)
{
    int opt;
    bool force = false;
    bool interactive = false;

    while(-1 != (opt = getopt(argc, argv, "f:a:t:n:d:i:x:rhv")))
    {
        switch(opt)
        {
            case 'f': //file name
                path = optarg;
                if(0 == strlen(path))
                    fail("File path cannot be empty\n");
                break;
            case 't': //type
                typeName = optarg;
                break;
            case 'n': //(existing) name
                name = optarg;
                if(0 == strlen(name))
                    fail("Entry name cannot be empty\n");
                break;
            case 'd': //value
                value = optarg;
                break;
            case 'i': //index
                indexa = optarg;
                break;
            case 'x': //new name
                newName = optarg;
                break;
            case 'r': //create if doesn't exist/skip creation if already exists
                force = true;
                break;
            case 'h': //help
                printf(helpPage);
                exit(0);
                break;
            case 'v':
                printf(VERSION_STRING);
                exit(0);
                break;
            default:
                fail("Unknown option %c\n", opt);
                break;
        }
    }


    if(optind < argc) //operation argument present?
    {
        if(NULL == path)
            fail("Database path must be provided\n");

        if(!strcmp(argv[optind], "new")) //new database
        {
            f = fopen(path, "rb+");
            if(NULL == f)
                newDb(path, &h);
            else
            {
                if(!force)
                    fail("File %s already exists\n", path);
                else
                {
                    fclose(f);
                    newDb(path, &h);
                }
            }
            
            printf("Empty database created\n");
            writeDb();
            NdbEditExit(0);
        }
        else //existing database
        {
            f = fopen(path, "rb+");
            if(NULL == f)
            {
                if(force)
                    newDb(path, &h);
                else
                    fail("File %s does not exists\n", path);
            }
            else
            {
                readDb(f, &h);
                printf("Opened file %s\n", path);
            }
        }

        strcpy(line, argv[optind]);
    }
    else
    {
        printf("Nabla OS database editor (interactive mode)\n\n");
        interactive = true;

        while(1)
        {
            printf("Enter file path: ");
            if(NULL == fgets(line, sizeof(line), stdin))
                fail("File path must be provided\n");
            
            line[strcspn(line, "\r\n")] = '\0';

            path = strdup(line);
            if(NULL == path)
                fail("Internal error in " FILE_LINE_STR "\n");
    
            f = fopen(line, "rb+");
            if(NULL == f)
            {
                printf("File %s does not exist, do you want to create it (Y/n)? ", path);
                if(GetYesNo(true))
                {
                    newDb(path, &h);
                    break;
                }
            }
            else
            {
                readDb(f, &h);
                printf("Opened file %s\n", path);
                //at this point, we should have the full file in memory
                break;
            }
        }
        free(path);
        path = NULL;
    }

    printf("Entries occupy %lu bytes\n", h->size);

    while(1)
    {
        if(interactive)
        {
            printf("> ");
            if(!get())
                continue;
        }
        //line buffer contains the input line in interactive mode or command argument in non-interactive mode

        if(!strcmp(line, "list"))
        {
            list(h);
            EXIT_ON_NON_INTERACTIVE(0);
        }
        else if(!strcmp(line, "add"))
        {
            bool isArrayElement = false;
            struct NablaDbEntry *array = NULL;
            struct NablaDbEntry *e = NULL;
            struct NablaDbArrayElement *ae = NULL;
            uint8_t *v = NULL;

            printf("Adding new entry\n");
            if(interactive)
            {
                printf("Add to existing array (y/N)? ");
                if(GetYesNo(false))
                {
                    isArrayElement = true;
                    printf("Existing array name: ");
                    if(!get())
                    {
                        printf("Array name must be provided\n");
                        continue;
                    }
                }
                name = line;
            }
            else
            {
                if(NULL == name)
                    fail("Entry name must be provided\n");
                
                array = NablaDbFind(h, name);
                if((NULL != array) && (NDB_ARRAY == array->type))
                {
                    isArrayElement = true;
                    printf("%s seems to be an array\n", name);
                }
            }
            
            if(isArrayElement && (NULL == array))
            {
                array = NablaDbFind(h, name);
                if(NULL == array)
                {
                    printf("No such array\n");
                    EXIT_ON_NON_INTERACTIVE(-1);
                    continue;
                }
            }

            if(interactive)
            {
                printf("Type: ");
                if(!get())
                {
                    printf("Type must be provided\n");
                    continue;
                }
                typeName = line;
            }
            else if(NULL == typeName)
                fail("Type must be provided\n");

            enum NablaDbType type = parseType(typeName);
            if(NDB_UNKNOWN == type)
            {
                printf("Unknown type %s\n", typeName);
                EXIT_ON_NON_INTERACTIVE(-1);
                continue;
            }

            if(!isArrayElement)
            {
                if(interactive)
                {
                    printf("Name: ");
                    if(!get() || ('\0' == line[0]))
                    {
                        printf("Entry name must be provided\n");
                        continue;
                    }
                    name = line;
                }
                else if(NULL == name)
                    fail("Entry name must be provided\n");

                if(NULL != NablaDbFind(h, name))
                {
                    printf("Entry %s already exists\n", name);
                    EXIT_ON_NON_INTERACTIVE(-1);
                    continue;
                }

                e = malloc(sizeof(*e) + strlen(name) + 1 + NablaDbGetTypeLength(type));
            }
            else
            {
                ae = malloc(sizeof(*ae) + NablaDbGetTypeLength(type));
                e = (struct NablaDbEntry*)ae;
            }

            if(NULL == e)
            {
                printf("Memory allocation failed\n");
                EXIT_ON_NON_INTERACTIVE(-1);
                continue;
            }

            if(!isArrayElement)
            {
                e->type = type;
                e->dataLength = NablaDbGetTypeLength(type);
                e->nameLength = strlen(name) + 1;
                strcpy(e->name, name);
                v = (uint8_t*)((uintptr_t)(e + 1) + e->nameLength);
            }
            else
            {
                ae->type = type | NABLADB_ARRAY_ELEMENT;
                ae->dataLength = NablaDbGetTypeLength(type);
                v = (uint8_t*)(ae + 1);
            }

            char *t = NULL;
            
            if(NDB_NULL != type)
            {
                if(interactive)
                {
                    printf("Value: ");
                    if(!get() || ('\0' == line[0]))
                    {
                        printf("Value must be provided\n");
                        continue;
                    }
                    value = line;
                }
                else if(NULL == value)
                    fail("Value must be provided\n");

                if(!getValue(value, type, &v, &e, NULL, false))
                    continue;
            }

            if(isArrayElement ? NablaDbAppendArrayElement(&h, array, ae) : NablaDbAppend(&h, e))
            {
                printf("Entry added\n");
                free(e);
                writeDb();
                EXIT_ON_NON_INTERACTIVE(0);
            }
            else
            {
                printf("Failed to add entry\n");
                free(e);
                EXIT_ON_NON_INTERACTIVE(-1);
            }
        }
        else if(!strcmp(line, "array"))
        {
            printf("Adding new array\n");
            
            if(interactive)
            {
                printf("Name: ");
                if(!get() || ('\0' == line[0]))
                {
                    printf("Array name must be provided\n");
                    continue;
                }
                name = line;
            }
            else if(NULL == name)
                fail("Array name must be provided\n");

            if(NULL != NablaDbFind(h, name))
            {
                printf("Entry %s already exists\n", name);
                EXIT_ON_NON_INTERACTIVE(-1);
                continue;
            } 
            
            struct NablaDbEntry *e = malloc(sizeof(*e) + strlen(name) + 1);
            if(NULL == e)
            {
                printf("Memory allocation failed\n");
                EXIT_ON_NON_INTERACTIVE(-1);
                continue;
            }

            e->type = NDB_ARRAY;
            e->elementCount = 0;
            e->nameLength = strlen(name) + 1;
            strcpy(e->name, name);

            if(NablaDbAppend(&h, e))
            {
                printf("Entry added\n");
                free(e);
                writeDb();
                EXIT_ON_NON_INTERACTIVE(0);
            }
            else
            {
                printf("Failed to add entry\n");
                free(e);
                EXIT_ON_NON_INTERACTIVE(-1);
            }
        }
        else if(!strcmp(line, "remove"))
        {
            printf("Removing entry\n");
            if(interactive)
            {
                printf("Name: ");
                if(!get() || ('\0' == line[0]))
                {
                    printf("Entry name must be provided\n");
                    continue;
                }
                name = line;
            }
            else if(NULL == name)
                fail("Entry name must be provided\n");

            struct NablaDbEntry *e = NablaDbFind(h, name);
            if(NULL == e)
            {
                printf("No such entry\n");
                EXIT_ON_NON_INTERACTIVE(-1);
                continue;
            }

            if(NDB_ARRAY == e->type)
            {
                if(interactive)
                    printf("%*s is an array. Do you want to remove the whole array (y/N)? ", e->nameLength, e->name);

                if((interactive && !GetYesNo(false)) || (!interactive && (NULL != indexa)))
                {
                    if(0 == e->elementCount)
                    {
                        printf("Array already empty, nothing to remove\n");
                        EXIT_ON_NON_INTERACTIVE(0);
                        continue;
                    }

                    struct NablaDbArrayElement *ae = getArrayEntry(interactive, indexa, h, e);
                    if(NULL == ae)
                    {
                        printf("No such entry\n");
                        EXIT_ON_NON_INTERACTIVE(-1);
                        continue;
                    }

                    if(NablaDbRemoveArrayElement(h, e, ae))
                    {
                        printf("Entry removed\n");
                        writeDb();
                        EXIT_ON_NON_INTERACTIVE(0);
                    }
                    else
                    {
                        printf("Failed to remove entry\n");
                        EXIT_ON_NON_INTERACTIVE(-1);
                    }
                    continue;
                }
            }

            if(NablaDbRemove(h, e))
            {
                printf("Entry removed\n");
                writeDb();
                EXIT_ON_NON_INTERACTIVE(0);
            }
            else
            {
                printf("Failed to remove entry\n");
                EXIT_ON_NON_INTERACTIVE(-1);
            }
        }
        else if(!strcmp(line, "name"))
        {
            printf("Changing entry name\n");
            if(interactive)
            {
                printf("Old name: ");
                if(!get() || ('\0' == line[0]))
                {
                    printf("Entry name must be provided\n");
                    continue;
                }
                name = line;
            }
            else if(NULL == name)
                fail("Entry name must be provided\n");

            struct NablaDbEntry *e = NablaDbFind(h, name);
            if(NULL == e)
            {
                printf("No such entry\n");
                EXIT_ON_NON_INTERACTIVE(-1);
                continue;
            }

            if(interactive)
            {
                printf("New name: ");
                if(!get() || ('\0' == line[0]))
                {
                    printf("New name must be provided\n");
                    continue;
                }
                newName = line;
            }
            else if(NULL == newName)
                fail("New name must be provided\n");

            if(NablaDbChangeName(&h, e, newName))
            {
                printf("Name changed\n");
                writeDb(); 
                EXIT_ON_NON_INTERACTIVE(0);
            }
            else
            {
                printf("Failed to change name\n");
                EXIT_ON_NON_INTERACTIVE(-1);
            }
        }
        else if(!strcmp(line, "value"))
        {
            printf("Changing entry value\n");
            
            if(interactive)
            {
                printf("Name: ");
                if(!get() || ('\0' == line[0]))
                {
                    printf("Entry name must be provided\n");
                    continue;
                }
                name = line;
            }
            else if(NULL == name)
                fail("Entry name must be provided\n");

            struct NablaDbEntry *e = NablaDbFind(h, name);
            if(NULL == e)
            {
                printf("No such entry\n");
                EXIT_ON_NON_INTERACTIVE(-1);
                continue;
            }

            if(NDB_ARRAY == e->type)
            {
                if(0 == e->elementCount)
                {
                    printf("Selected entry is an empty array, nothing to change\n");
                    EXIT_ON_NON_INTERACTIVE(-1);
                    continue;
                }

                e = (struct NablaDbEntry*)getArrayEntry(interactive, indexa, h, e);
                if(NULL == e)
                {
                    printf("No such entry\n");
                    EXIT_ON_NON_INTERACTIVE(-1);
                    continue;
                }
            }

            union NablaDbData newValue;
            uint32_t length = 0;

            uint8_t *v = NULL;
            if(!NablaDbIsVariableLength(e->type))
            {
                v = malloc(NablaDbGetTypeLength(e->type));
                if(NULL == v)
                {
                    printf("Memory allocation failed\n");
                    EXIT_ON_NON_INTERACTIVE(-1);
                    continue;
                }
            }

            if(interactive)
            {
                printf("New value: ");
                if(!get() || ('\0' == line[0]))
                {
                    printf("Value must be provided\n");
                    continue;
                }
                value = line;
            }
            else if(NULL == value)
                fail("Value must be provided\n");

            if(!getValue(value, e->type, &v, NULL, &length, true))
            {
                free(v);
                EXIT_ON_NON_INTERACTIVE(-1);
                continue;
            }

            switch(NABLADB_TYPE_MASK(e->type))
            {
                case NDB_UTF8:
                    newValue.utf8 = (char*)v;
                    break;
                case NDB_MULTI:
                    newValue.multi = v;
                    break;
                case NDB_UUID:
                    newValue.uuid = v;
                    break;
                default:
                    newValue = *((union NablaDbData*)v);
                    break;
            }

            if(NablaDbChangeValue(&h, e, newValue, length))
            {
                printf("Value changed\n");
                writeDb();
                EXIT_ON_NON_INTERACTIVE(0);
            }
            else
            {
                printf("Failed to change value\n");
                EXIT_ON_NON_INTERACTIVE(-1);
            }

        }
        else if(!strcmp(line, "save"))
        {
            writeDb();
            EXIT_ON_NON_INTERACTIVE(0);
        }
        else if(!strcmp(line, "exit"))
        {
            writeDb();
            NdbEditExit(0);
        }
        else
        {
            printf("Unknown command %s\n", line);
            if(!interactive)
                printf("Use %s -h to show help page\n", argv[0]);
            EXIT_ON_NON_INTERACTIVE(-1);
        }
    }
}
