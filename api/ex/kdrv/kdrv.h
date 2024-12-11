//This header file is generated automatically
#ifndef EXPORTED___API__EX_KDRV_KDRV_H_
#define EXPORTED___API__EX_KDRV_KDRV_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include <stdbool.h>
#include "ob/ob.h"

struct ExDriverObject;
struct IoDeviceObject;
struct IoRp;
struct IoVolumeNode;


#define DRIVER_ENTRY DriverEntry
typedef STATUS DRIVER_ENTRY_T(struct ExDriverObject *);

/**
 * @brief Driver was initialized successfully
 * @warning This flag is set and cleared by the kernel
 */
#define EX_DRIVER_OBJECT_FLAG_INITIALIZED 0x80000000

/**
 * @brief Driver is a filesystem driver
 * @warning This flag must be set by the driver in \a DriverEntry() routine
 */
#define EX_DRIVER_OBJECT_FLAG_FILESYSTEM 0x00000001

/**
 * @brief Driver object structure
 */
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
    STATUS (*verifyFs)(struct ExDriverObject *driverObject, struct IoDeviceObject *disk);
    STATUS (*mount)(struct ExDriverObject *driverObject, struct IoDeviceObject *disk);

    char *imageName;
    struct ExDriverObject *next;
    struct ExDriverObject *previous;
};

/**
 * @brief Helper driver object list passed to \a ExLoadKernelDriver...() caller
 */
struct ExDriverObjectList
{
    struct ExDriverObject *this;
    struct ExDriverObjectList *next;
    bool isMain;
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


#ifdef __cplusplus
}
#endif

#endif