//This header file is generated automatically
#ifndef EXPORTED___API__SDRV_DCPUID_H_
#define EXPORTED___API__SDRV_DCPUID_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdbool.h>
#include <stdint.h>
#include "defines.h"
/**
 * @brief Get CPU vendor string (13 characters incl. NULL terminator)
 * @param *dst Destination buffer
*/
extern void CpuidGetVendorString(char *dst);


#ifdef __cplusplus
}
#endif

#endif