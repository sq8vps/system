//This header file is generated automatically
#ifndef EXPORTED___API__OB_OB_H_
#define EXPORTED___API__OB_OB_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "ke/core/mutex.h"
#include "defines.h"

/**
 * @brief Header structure for all kernel objects
*/
struct ObObjectHeader
{
    uint32_t magic;
    KeSpinlock lock;
};


#ifdef __cplusplus
}
#endif

#endif