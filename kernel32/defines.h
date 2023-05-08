#ifndef KERNEL32_DEFINES_H_
#define KERNEL32_DEFINES_H_

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef enum kError_t
{
    //OK response
    OK = 0,

    //interrupt module errors
    IT_BAD_VECTOR = 0x00001000, //bad interrupt vector number

    //memory management module errors
    MM_NO_MEMORY = 0x00002000, //out of physical memory
    MM_PAGE_NOT_PRESENT = 0x00002001, //page not present in physical memory
    MM_ALREADY_MAPPED = 0x00002002, //page is already mapped to other virtual address

} kError_t;

#endif