#ifndef KERNEL_DEV_H_
#define KERNEL_DEV_H_

#include <stdint.h>
#include "defines.h"
#include "ex/kdrv.h"

typedef uint32_t IoFlags;
typedef uint8_t IoIrpCode;
#define IO_DEVICE_FLAG_INITIALIZED 0x1
#define IO_DEVICE_FLAG_INITIALIZATION_FAILURE 0x2
#define IO_DEVICE_FLAG_PERSISTENT 0x80000000
#define IO_DEVICE_FLAG_REMOVABLE_MEDIA 0x4


#define IO_SUBDEVICE_FLAG_PERSISTENT (IO_DEVICE_FLAG_PERSISTENT)

enum IoDeviceType
{
    IO_DEVICE_TYPE_NONE = 0, //dummy driver
    IO_DEVICE_TYPE_OTHER, //other devices
    IO_DEVICE_TYPE_ROOT, //system root hardware (ACPI, MP)
    IO_DEVICE_TYPE_BUS, //bus (PCI, PCIe, ISA...)
    IO_DEVICE_TYPE_DISK, //disk (IDE, AHCI, NVMe...)
    IO_DEVICE_TYPE_PARTMGMT, //partition management (MBR, GPT)
    IO_DEVICE_TYPE_FS, //filesystem (EXT, FAT...)
};

struct IoDeviceSubObject;
struct IoDriverIrp;

typedef STATUS (*IoCompletionCallback)(struct IoDeviceSubObject *device, struct IoDriverIrp *irp, void *context);

struct IoDriverIrp
{
    struct IoDeviceSubObject *deviceObject;
    IoIrpCode code;
    IoFlags flags;
    void *buffer;
    uint64_t size;
    STATUS status;
    union
    {
        
    } data;
    IoCompletionCallback completionCallback;
    void *completionContext;
};

struct IoDeviceObject
{
    char *deviceId;
    char *name;
    IoFlags flags;
    struct IoDeviceSubObject *baseDevice;
    struct IoDeviceObject *parent;
};

struct IoDeviceSubObject
{
    enum IoDeviceType type;
    struct ExDriverObject *driverObject;
    struct IoDeviceSubObject *nextDevice; //next device created by the same driver
    struct IoDeviceSubObject *attachedTo;
    struct IoDeviceSubObject *attachedDevice;
    struct IoDriverIrp *currentIrp;
    uint32_t referenceCount;
    IoFlags flags;
    void *privateData;
    struct IoDeviceObject *mainDeviceObject;
};

/**
 * @brief Create device subobject from driver
 * @param *driver Driver object pointer
 * @param privateSize Size of \a (*device)->privateData to allocate
 * @param type Device type. Must be the same in main and all supplementary device subobjects.
 * @param flags Device flags
 * @param **device Output subdevice object pointer. The memory is allocated by this function.
 * @return Status code
 * @attention Created device must be attached to the stack using \a IoAttachSubDevice().
*/
STATUS IoCreateSubDevice(
    struct ExDriverObject *driver, 
    uint32_t privateSize, 
    enum IoDeviceType type, 
    IoFlags flags, 
    struct IoDeviceSubObject **device);

/**
 * @brief Attach subdevice to the top of the device stack
 * @param *attachee Subdevice object to be attached
 * @param *destination Any subdevice object in given device stack
 * @return Pointer to the previous top device stack object or NULL on failure
*/
struct IoDeviceSubObject* IoAttachSubDevice(
    struct IoDeviceSubObject *attachee, 
    struct IoDeviceSubObject *destination);

/**
 * @brief Register new device to be used by the system and load appropriate drivers
 * @param *baseDevice Base (bus driver) subdevice object
 * @param *deviceId Device ID string
 * @return Status code
 * @warning This function should be called only by (or on behalf of) base (bus) subdevice object
*/
STATUS IoRegisterDevice(struct IoDeviceSubObject *baseDevice, char *deviceId);

/**
 * @brief Initialize device manager and create root device
 * @param *rootDeviceId Root device ID string (to find appropriate driver)
 * @return Status code
*/
STATUS IoInitDeviceManager(char *rootDeviceId);

STATUS IoSetDeviceDisplayedName(struct IoDeviceSubObject *device, char *name);

#endif