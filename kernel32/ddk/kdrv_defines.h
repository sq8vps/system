#ifndef DDK32_KDRV_DEFINES_H_
#define DDK32_KDRV_DEFINES_H_

#include <stdint.h>

/**
 * @brief Kernel mode driver main classes
*/
typedef enum 
{
    DDK_KDRVCLASS_NONE = 0, //dummy driver class
    DDK_KDRVCLASS_DISK = 1, //disk driver
    DDK_KDRVCLASS_FS = 2, //filesystem driver
    DDK_KDRVCLASS_SCREEN = 3, //screen driver
    DDK_KDRVCLASS_KEYBOARD = 4, //keyboard driver
} DDK_KDrvClass;

#define DDK_KDRVMETADATA_STRING_SIZE 50 //max string size in driver metadata structure

/**
 * @brief Kernel mode driver metadata structure
 * @attention Must be defined in the driver
*/
typedef struct
{
    char name[DDK_KDRVMETADATA_STRING_SIZE + 1];
    char vendor[DDK_KDRVMETADATA_STRING_SIZE + 1];
    char version[DDK_KDRVMETADATA_STRING_SIZE + 1];
    DDK_KDrvClass class;
} DDK_KDrvMetadata_t;

/**
 * @brief Driver entry point name macro
 * @attention Use this macro as driver entry point function name
*/
#define KDRV_ENTRY kdrv_entry

/**
 * @brief Driver metadata structure name macro
 * @attention Use tis macro as driver metadata structure name
*/
#define KDRV_METADATA kdrv_metadata

#endif