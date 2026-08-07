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

DRIVER_API

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
 * @brief Driver object flags
 */
enum ExDriverObjectFlags
{
    EX_DRIVER_OBJECT_FLAG_FILESYSTEM  = 0x00000001, /**< Driver is a filesystem driver */
    EX_DRIVER_OBJECT_FLAG_LOADED = 0x80000000, /**< Driver is already loaded and its entry was called */
};

/**
 * @brief Driver entry point type
 * @param *driverObject Target driver object
 * @param *dbPath Path of the database that was used to load this driver
 * @return Status code
 * @attention This function is called only once and only by the kernel
 */
typedef STATUS (*ExDriverEntry)(struct ExDriverObject *driverObject, const char *dbPath);

/**
 * @brief High-level driver initialization routine type
 * 
 * This routine is implemented by high-level drivers for intialization when a low-level driver requests
 * support from a high-level driver. This routine may (and probably will) read the low-level driver (caller) routine pointers
 * to pass some requests later, so the low-level driver may modify them (for example to direct all requests to the high-level driver first)
 * only after calling this function.
 * @param *driverObject Target driver object
 * @param *caller Caller driver object
 * @param *data Driver-specific data
 * @return Status code
 */
typedef STATUS (*ExDriverInit)(struct ExDriverObject *driverObject, struct ExDriverObject *caller, void *data);

/**
 * @brief Driver RP dispatch routine type
 * 
 * This function is used to pass an RP to a driver.
 * @param *rp RP to be dispatched by the driver
 * @return Status code
 */
typedef STATUS (*ExDriverDispatch)(struct IoRp *rp);

/**
 * @brief Add device driver dispatch routine
 * 
 * This function is called by the kernel when a new device was enumerated, and the device ID matches at least one
 * of the device IDs supported by the driver.
 * @param *driverObject Target driver object
 * @param *baseDeviceObject Base device object, to which the newly created device should be attached
 * @return Status code
 */
typedef STATUS (*ExDriverAddDevice)(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject);

/**
 * @brief Driver object structure
 */
struct ExDriverObject
{
    OBJECT;
    bool free; /**< Descriptor is free */
    uint32_t id; /**< Unique driver ID */
    struct IoDeviceObject *deviceObject; /**< Linked list of devices created by the driver */
    enum ExDriverObjectFlags flags; /**< Driver flags */
    uintptr_t address; /**< Driver image address */
    size_t size; /**< Driver image size */
    uint32_t referenceCount; /**< Count of driver references */
    ExDriverEntry entry; /**< Driver entry routine */
    ExDriverInit init; /**< Driver specific initialization function - not called by default */
    STATUS (*unload)(struct ExDriverObject *driverObject); /**< Driver unload routine pointer */
    ExDriverDispatch dispatch; /**< Resource Packet dispatch routine pointer */
    ExDriverAddDevice addDevice; /**< Main Device Object creation routine pointer */
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
    struct ExDriverObject *thisDriver; /**< Associated driver object pointer */
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
STATUS ExLoadKernelDriversForDevice(const char *deviceId, char * const *compatibleIds, struct ExDriverObjectList **drivers, size_t *driverCount);

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
STATUS ExLoadKernelDriversForFilesystem(struct IoVolumeNode *volume, struct ExDriverObjectList **drivers, size_t *driverCount);

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
STATUS ExLoadKernelDriversByName(const char *name, struct ExDriverObjectList **drivers, size_t *driverCount);

/**
 * @brief Find driver by memory address (e.g. for debugging)
 * @param *address Input/output pointer to memory address buffer. The driver base address is returned to this variable.
 * @return Matched driver object or NULL if no matching object was found
*/
struct ExDriverObject *ExFindDriverByAddress(uintptr_t *address);

END_DRIVER_API

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