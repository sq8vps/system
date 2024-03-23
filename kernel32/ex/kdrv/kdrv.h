#ifndef KERNEL_KDRV_H_
#define KERNEL_KDRV_H_

/**
 * @file kdrv.h
 * @brief Kernel mode driver library
 * 
 * Provides kernel mode drivers manipulation routines
*/

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>
#include "ob/ob.h"

/**
 * @defgroup kdrvLoad Kernel mode driver loading
 * @ingroup exec
 * @{
*/

EXPORT
struct ExDriverObject;
struct IoDeviceObject;
struct IoRp;

EXPORT
#define DRIVER_ENTRY DriverEntry
typedef STATUS DRIVER_ENTRY_T(struct ExDriverObject *);

EXPORT
struct ExDriverObject
{
    struct ObObjectHeader objectHeader;
    bool free;
    uint32_t id;
    struct IoDeviceObject *deviceObject; //linked list of devices created by the driver
    uint32_t flags;
    uintptr_t address;
    uintptr_t size;
    uint32_t referenceCount;
    STATUS (*init)(struct ExDriverObject *driverObject);
    STATUS (*unload)(struct ExDriverObject *driverObject);
    STATUS (*dispatch)(struct IoRp *rp);
    STATUS (*addDevice)(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject);
    STATUS (*mount)(struct ExDriverObject *driverObject, struct IoDeviceObject *disk);

    struct ExDriverObject *next;
    struct ExDriverObject *previous;
};

EXPORT
struct ExDriverObjectList
{
    struct ExDriverObject *this;
    struct ExDriverObjectList *next;
};

/**
 * @brief Load and initialize drivers for given device ID
 * @param *deviceId Device ID string
 * @param **drivers Dynamically allocated table of loaded driver objects pointers
 * @param *driverCount Number of loaded drivers
 * @return Status code. \a **drivers is NULL when function fails.
 * @warning This function fails when at least one required driver was not loaded.
 * @note Freeing the \a **drivers table must be done by the caller.
 * @note This function reuses already loaded drivers.
*/
STATUS ExLoadKernelDriversForDevice(const char *deviceId, struct ExDriverObjectList **drivers, uint16_t *driverCount);

/**
 * @brief Load given driver
 * @param *path Driver file path
 * @param **driverObject Output pointer with created driver object
 * @return Error code
*/
INTERNAL STATUS ExLoadKernelDriver(char *path, struct ExDriverObject **driverObject);

EXPORT
/**
 * @brief Find driver by memory address (e.g. for debugging)
 * @param *address Input/output pointer to memory address buffer. The driver base address is returned to this variable.
 * @return Matched driver object or NULL if no matching object was found
*/
EXTERN struct ExDriverObject *ExFindDriverByAddress(uintptr_t *address);

/**
 * @}
*/


#endif