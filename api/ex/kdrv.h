//This header file is generated automatically
#ifndef EXPORTED___API__EX_KDRV_H_
#define EXPORTED___API__EX_KDRV_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "io/devmgr/dev.h"
struct ExDriverObject;
struct IoDeviceSubObject;
struct IoDeviceSubObject;
struct IoDriverIRP;

#define DRIVER_ENTRY DriverEntry
typedef STATUS DRIVER_ENTRY_T(struct ExDriverObject *);

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


#ifdef __cplusplus
}
#endif

#endif