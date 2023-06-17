#ifndef KERNEL32_DEFINES_H_
#define KERNEL32_DEFINES_H_

#ifndef NULL
#define NULL ((void*)0)
#endif

#define STRINGIFY(s) #s
#define EXPAND_AND_STRINGIFY(s) STRINGIFY(s)

typedef enum kError_t
{
    //OK response
    OK = 0,

    //common errors
    NULL_POINTER_GIVEN = 1,

    //interrupt module errors
    IT_BAD_VECTOR = 0x00001000, //bad interrupt vector number

    //memory management module errors
    MM_NO_MEMORY = 0x00002000, //out of physical memory
    MM_PAGE_NOT_PRESENT, //page not present in physical memory
    MM_ALREADY_MAPPED, //page is already mapped to other virtual address

    EXEC_ELF_BAD_ARCHITECTURE = 0x00003000,
    EXEC_ELF_BAD_INSTRUCTION_SET,
    EXEC_ELF_BAD_FORMAT,
    EXEC_ELF_BAD_ENDIANESS,
    EXEC_ELF_BROKEN,
    EXEC_ELF_UNSUPPORTED_TYPE,
    EXEC_ELF_UNDEFINED_SYMBOL,
    EXEC_ELF_UNDEFINED_EXTERNAL_SYMBOL,
    EXEC_MALLOC_FAILED,
    EXEC_UNSUPPORTED_KERNEL_MODE_DRIVER_TYPE,
    EXEC_BAD_DRIVER_INDEX,
    EXEC_BAD_DRIVER_CLASS,


} kError_t;



#endif