//This header file is generated automatically
#ifndef EXPORTED___API__EX_KDRV_H_
#define EXPORTED___API__EX_KDRV_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include <stdarg.h>
#include "ob/ob.h"
struct ExDriverObject;
struct IoDeviceObject;
struct IoRp;

#define DRIVER_ENTRY DriverEntry
typedef STATUS DRIVER_ENTRY_T(struct ExDriverObject *);

struct ExDriverObject
{
    struct ObObjectHeader objectHeader;
    uint32_t index;
    struct IoDeviceObject *deviceObject; //linked list of devices created by the driver
    uint32_t flags;
    uintptr_t address;
    uintptr_t size;
    STATUS (*init)(struct ExDriverObject *driverObject);
    STATUS (*unload)(struct ExDriverObject *driverObject);
    STATUS (*dispatch)(struct IoRp *rp);
    STATUS (*addDevice)(struct ExDriverObject *driverObject, struct IoDeviceObject *baseDeviceObject);
    STATUS (*mount)(struct ExDriverObject *driverObject, struct IoDeviceObject *disk);

    struct ExDriverObject *next;
    struct ExDriverObject *previous;
};

struct ExDriverObjectList
{
    struct ExDriverObject *this;
    struct ExDriverObjectList *next;
};

/**
 * @brief Find driver by memory address (e.g. for debugging)
 * @param *address Input/output pointer to memory address buffer. The driver base address is returned to this variable.
 * @return Matched driver object or NULL if no matching object was found
*/
extern struct ExDriverObject *ExFindDriverByAddress(uintptr_t *address);


#ifdef __cplusplus
}
#endif

#endif