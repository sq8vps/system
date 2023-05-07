#ifndef KERNEL32_DEFINES_H_
#define KERNEL32_DEFINES_H_

typedef enum kError_t
{
    //OK response
    OK = 0,

    //interrupt module errors
    IT_BAD_VECTOR = 0x00001000,


} kError_t;

#endif