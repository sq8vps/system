/**
 * @file kdrv.h
 * @brief Kernel mode driver/device specific routines
 * @ingroup kdrv
 */

#ifndef KERNEL_KDRV_H_
#define KERNEL_KDRV_H_

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>
#include "ob/ob.h"

EXPORT_API

struct ExDriverObject;
struct IoDeviceObject;
struct IoRp;
struct IoVolumeNode;

/**
 * @addtogroup kdrv Kernel mode driver handling routines
 * @ingroup exec
 * @{
*/

/**
 * @brief Macro to be used as a kernel mode driver entry point name
 */
#define DRIVER_ENTRY DriverEntry

/**
 * @brief Type definition for a kernel mode driver entry point routine
 */
typedef STATUS DRIVER_ENTRY_T(struct ExDriverObject *);

/**
 * @brief Driver was initialized successfully
 * @warning This flag is set and cleared by the kernel
 */
#define EX_DRIVER_OBJECT_FLAG_INITIALIZED 0x80000000

/**
 * @brief Driver is a filesystem driver
 * @warning This flag must be set by the driver in #DRIVER_ENTRY routine
 */
#define EX_DRIVER_OBJECT_FLAG_FILESYSTEM 0x00000001

/**
 * @brief Driver object structure
 */
struct ExDriverObject
{
    OBJECT;
    bool free; /**< Descriptor is free */
    uint32_t id; /**< Unique driver ID */
    struct IoDeviceObject *deviceObject; /**< Linked list of devices created by the driver */
    uint32_t flags; /**< Driver flags */
    uintptr_t address; /**< Driver image address */
    size_t size; /**< Driver image size */
    uint32_t referenceCount; /**< Count of driver references */
    STATUS (*init)(struct ExDriverObject *driverObject); /**< Driver initialization routine pointer */
    STATUS (*unload)(struct ExDriverObject *driverObject); /**< Driver unload routine pointer */
    STATUS (*dispatch)(struct IoRp *rp); /**< Resource Packet dispatch routine pointer */
    STATUS (*addDevice)(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject); /**< Main Device Object creation routine pointer */
    STATUS (*verifyFs)(struct ExDriverObject *driverObject, struct IoDeviceObject *disk); /**< Routine to check if driver is able to mount a given file system */
    STATUS (*mount)(struct ExDriverObject *driverObject, struct IoDeviceObject *disk); /**< Mount filesystem routine pointer */

    char *imageName; /**< Image file name */
    struct ExDriverObject *next; /**< Next driver object */
    struct ExDriverObject *previous; /**< Previous driver object */
};

/**
 * @brief Helper driver object list passed to \a ExLoadKernelDriver...() caller
 */
struct ExDriverObjectList
{
    struct ExDriverObject *this; /**< Associated driver object pointer */
    struct ExDriverObjectList *next; /**< Next driver object list entry */
    bool isMain; /**< Is this a main driver for this device? */
};

/**
 * @brief Load and initialize drivers for given device ID
 * @param *deviceId Device ID string
 * @param **compatbileIds Compatible IDs table of strings
 * @param **drivers Dynamically allocated table of loaded driver objects pointers
 * @param *driverCount Number of loaded drivers
 * @return Status code. \a **drivers is NULL when function fails.
 * @warning This function fails when at least one required driver was not loaded.
 * @note Freeing the \a **drivers table must be done by the caller.
 * @note This function reuses already loaded drivers.
*/
STATUS ExLoadKernelDriversForDevice(const char *deviceId, char * const *compatibleIds, struct ExDriverObjectList **drivers, uint16_t *driverCount);

/**
 * @brief Load and initialize filesystem drivers for given volume
 * @param *volume Volume node
 * @param **drivers Dynamically allocated table of loaded driver objects pointers
 * @param *driverCount Number of loaded drivers
 * @return Status code. \a **drivers is NULL when function fails.
 * @warning This function fails when at least one required driver was not loaded.
 * @note Freeing the \a **drivers table must be done by the caller.
 * @note This function reuses already loaded drivers.
*/
STATUS ExLoadKernelDriversForFilesystem(struct IoVolumeNode *volume, struct ExDriverObjectList **drivers, uint16_t *driverCount);

/**
 * @brief Load and initialize drivers described by given database
 * @param *name Database file name
 * @param **drivers Dynamically allocated table of loaded driver objects pointers
 * @param *driverCount Number of loaded drivers
 * @return Status code. \a **drivers is NULL when function fails.
 * @warning This function fails when at least one required driver was not loaded.
 * @note Freeing the \a **drivers table must be done by the caller.
 * @note This function reuses already loaded drivers.
*/
STATUS ExLoadKernelDriversByName(const char *name, struct ExDriverObjectList **drivers, uint16_t *driverCount);

/**
 * @brief Find driver by memory address (e.g. for debugging)
 * @param *address Input/output pointer to memory address buffer. The driver base address is returned to this variable.
 * @return Matched driver object or NULL if no matching object was found
*/
struct ExDriverObject *ExFindDriverByAddress(uintptr_t *address);

END_EXPORT_API

/**
 * @brief Initialize driver manager
 * @return Status code
 * @kinternal
 */
INTERNAL STATUS ExInitializeDriverManager(void);

/**
 * @brief Update driver database path after the main file system is mounted
 * @return Status code
 * @warning This function might be called only once
 * @kinternal
 */
INTERNAL STATUS ExUpdateDriverDatabasePath(void);

/**
 * @}
*/


#endif