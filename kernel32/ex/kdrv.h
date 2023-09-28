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
#include "io/devmgr/dev.h"


/**
 * @defgroup kdrvLoad Kernel mode driver loading
 * @ingroup exec
 * @{
*/

EXPORT
struct ExDriverObject;
struct IoDeviceSubObject;
struct IoDeviceSubObject;
struct IoDriverIRP;

EXPORT
#define DRIVER_ENTRY DriverEntry
typedef STATUS DRIVER_ENTRY_T(struct ExDriverObject *);

EXPORT
struct ExDriverObject
{
    uint32_t index;
    struct IoDeviceSubObject *deviceObject; //linked list of devices created by the driver
    uint32_t flags;
    uintptr_t address;
    uintptr_t size;
    char *name;
    char *vendor;
    char *version;
    void *driverData;
    STATUS (*driverInit)(struct ExDriverObject *driverObject);
    STATUS (*driverUnload)(struct ExDriverObject *driverObject);
    STATUS (*driverDispatchIrp)(struct IoDriverIRP *irp);
    STATUS (*driverAddDevice)(struct ExDriverObject *driverObject, struct IoDeviceSubObject *baseDeviceObject);

    struct ExDriverObject *next;
    struct ExDriverObject *previous;
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
STATUS ExLoadKernelDriversForDevice(const char *deviceId, struct ExDriverObject **drivers, uint16_t *driverCount);

/**
 * @brief Relocate, link and execute preloaded driver
 * @param driverImage Raw driver image address
 * @param size Bytes occupied by the driver
 * @return Error code
 * @attention Full raw driver image must be preloaded to memory. All no-bits/BSS sections must be allocated before.
*/
STATUS ExLoadPreloadedDriver(const uintptr_t driverImage, const uintptr_t size);


/**
 * @}
*/


#endif