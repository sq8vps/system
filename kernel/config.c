#include "config.h"
#include "rtl/string.h"

#ifdef MULTIBOOT2
#include "multiboot/multiboot.h"
#endif

/**
 * @brief Known kernel parameter names
 */
static const char *KnownParams[] = {
    "initrd", //initrd module name
    "init", //init program path
};

#define KERNEL_PARAM_COUNT (sizeof(KnownParams) / sizeof(KnownParams[0])) /**< Maximum number of stored kernel params */


static struct
{
    const char *name;
    const char *value;
}
KernelParam[KERNEL_PARAM_COUNT]; /**< Kernel parameters: name - value pairs */

static const char *InitArgv[MAX_INIT_ARGS + 1] = {NULL}; /**< Init arguments */
static size_t InitArgvCount = 1; /**< Number of entries in @ref InitArgvCount. 1st argument is reserved for program name. */

/**
 * @brief Store parameter without value as kernel parameter or init parameter
 * @param *name Parameter name
 * @param initArg True if should be treated as init parameter, false if stored as kernel parameter
 */
static void AddWithoutValue(const char *name, bool initArg)
{
    if(initArg)
    {
        if(InitArgvCount < MAX_INIT_ARGS)
        {
            InitArgv[InitArgvCount++] = name;
            InitArgv[InitArgvCount] = NULL;
        }
    }
    else
    {
        for(size_t i = 0; i < KERNEL_PARAM_COUNT; i++)
        {
            if(0 == strcmp(KnownParams[i], name))
            {
                if(NULL == KernelParam[i].name)
                    KernelParam[i].name = name;
            }
        }
    }
}

/**
 * @brief Store parameter with value as kernel parameter or user parameter
 * @param *name Parameter name
 * @param *value Parameter value
 * @return True if stored as kernel parameter, false if stored as user parameter
 */
static bool AddWithValue(const char *name, const char *value)
{
    for(size_t i = 0; i < KERNEL_PARAM_COUNT; i++)
    {
        if(0 == strcmp(KnownParams[i], name))
        {
            if(NULL == KernelParam[i].name)
            {
                KernelParam[i].name = name;
                KernelParam[i].value = value;
            }
            return true;
        }
    }

    if(InitArgvCount < MAX_INIT_ARGS)
    {
        InitArgv[InitArgvCount++] = name;
        InitArgv[InitArgvCount] = NULL;
    }
    return false;
}

void ConfigParseKernelArguments(void *bootData)
{
    char *line = NULL;
#ifdef MULTIBOOT2
    const struct Multiboot2InfoTag *tag = Multiboot2FindTag(bootData, NULL, MB2_COMMAND_LINE);
    if(NULL == tag)
        return;
    struct Multiboot2CommandLineTag *cl = (struct Multiboot2CommandLineTag*)tag;
    line = cl->str; 
#endif

    memset(KernelParam, 0, sizeof(KernelParam));

    if(NULL == line)
        return;

    const char *end = line + strlen(line);
    char *name = NULL;
    bool initArgs = false;

    while(line < end)
    {
        //skip whitespaces
        while(' ' == *line)
        {
            if(end == ++line)
                return;
        }

        name = line;
        //look for end of name
        ++line;
        while(1)
        {
            if(('\0' == *line) || (line == end)) //end of data, no value
            {
                *line = '\0';
                if('"' == *name)
                    ++name;
                AddWithoutValue(name, initArgs);
                return;
            }
            else if('=' == *line) //value will follow
            {
                if(((line - name) >= 2) && (('-' == name[0]) && ('-' == name[1])))
                {
                    line = name + 2;
                    initArgs = true;
                    break;
                }
                *line++ = '\0';
                if(('\0' == *line) || (line == end)) //dodgy case - end of line
                {
                    if('"' == *name)
                        ++name;
                    AddWithoutValue(name, initArgs); //treat as kernel or user argument
                    return;
                }
                else if((' ' == *line) && ('"' != *name)) //expected value, but no value - fine
                {
                    AddWithoutValue(name, initArgs); //treat as kernel or user argument
                }
                else
                {
                    /*
                    Basically, a name=value entry can be either a kernel parameter or an user environment variable
                    depending on if the parameter name is a well known kernel parameter name.
                    If so, name and value are separated and "=" sign is replaced with NULL. 
                    If not, the whole name=value line must be passed to the user program.
                    But we need a null-terminated name string, so "=" must be temporarily replaced anyway.
                    */
                    char *value = line;

                    while(!(('\0' == *line) || (line == end) 
                        || (('"' == *name) && ('"' == *line)) 
                        || (('"' != *name) && (' ' == *line))))
                    {
                        ++line;
                    }
                    *line = '\0';
                    if('"' == *name)
                        ++name;
                    if(!AddWithValue(name, value))
                        value[-1] = '=';
                    ++line;
                }
                break;
            }
            else if(((' ' == *line) && ('"' != *name))
                ||  (('"' == *line) && ('"' == *name)))
            {
                if(((line - name) >= 2) && (('-' == name[0]) && ('-' == name[1])))
                {
                    line = name + 2;
                    initArgs = true;
                    break;
                }
                *line++ = '\0';
                if('"' == *name)
                    ++name;
                AddWithoutValue(name, initArgs);
                break;
            }
            else
                ++line;
        }
    }
}

bool ConfigGetKernelParam(const char *name, const char **value)
{
    for(size_t i = 0; i < KERNEL_PARAM_COUNT; i++)
    {
        if((NULL != KernelParam[i].name) && (0 == strcmp(name, KernelParam[i].name)))
        {
            if(NULL != value)
                *value = KernelParam[i].value;
            return true;
        }
    }
    return false;
}

size_t ConfigGetUserParams(const char*** args)
{
    if(nullptr != args)
        *args = InitArgv;
    return InitArgvCount;
}