#ifndef KERNEL_EXPORT_H_
#define KERNEL_EXPORT_H_

#include "defines.h"

#define _EXPORT_TABLE_NAME .ksymbols
#define EXPORT_TABLE_NAME STRINGIFY(_EXPORT_TABLE_NAME)

/**
 * @brief Mark function/variable declaration as "to be exported"
*/
#define EXPORT

#ifndef AMD64
    #define _EXPORT_SYMBOL(symbol) \
        .section _EXPORT_TABLE_NAME ; \
        .ascii STRINGIFY(symbol) "\0" ; \
        .long symbol ;
#else
    #define _EXPORT_SYMBOL(symbol) \
        .section _EXPORT_TABLE_NAME ; \
        .ascii STRINGIFY(symbol) "\0" ; \
        .quad symbol ;
#endif

/**
 * @brief Export symbol to be used by kernel modules
*/
#define EXPORT_SYMBOL(symbol) \
    asm(STRINGIFY(_EXPORT_SYMBOL(symbol)))



#endif