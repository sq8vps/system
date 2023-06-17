#ifndef DDK32_KDRV_DEFINES_H_
#define DDK32_KDRV_DEFINES_H_

/**
 * @file kdrv_defines.h
 * @brief Kernel mode driver definitions
 * 
 * Provides general definitions for all kernel mode drivers
 * 
*/

#include <stdint.h>

/**
 * @defgroup ddkDefines Driver development kit general definitions
 * @ingroup ddk
 * @{
*/

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
} DDK_KDrvClass_t;

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
    DDK_KDrvClass_t class;
} DDK_KDrvMetadata_t;

/**
 * @brief Driver entry point name macro
 * @warning Do not use
*/
#define KDRV_ENTRY_NAME kdrv_entry

/**
 * @brief Driver index type
*/
typedef uintptr_t KDRV_INDEX_T;

/**
 * @brief Driver entry macro. Define it and use as an entry point in your driver.
 * @param INDEX The driver index which is assigned by the kernel.
*/
#define KDRV_ENTRY(INDEX) void KDRV_ENTRY_NAME(KDRV_INDEX_T INDEX)

/**
 * @brief Driver metadata structure name macro
 * @warning Do not use
*/
#define KDRV_METADATA_NAME kdrv_metadata

/**
 * @brief Driver metadata macro. Define and fill it in your driver.
*/
#define KDRV_METADATA DDK_KDrvMetadata_t KDRV_METADATA_NAME

/**
 * @brief General driver callback structure
*/
typedef struct
{
    /**
     * @brief Driver start function
    */
    void (*start)();
    /**
     * @brief Driver stop function
    */
    void (*stop)();
} DDK_KDrvGeneralCallbacks_t;


/**
 * @}
*/

#endif