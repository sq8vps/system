#ifndef KERNEL_RP_H_
#define KERNEL_RP_H_

#include <stdint.h>
#include "defines.h"
#include "dev.h"
#include "typedefs.h"
#include "ke/core/mutex.h"
#include "types.h"

EXPORT
struct IoDriverRp;
struct IoRpQueue;
struct IoSubDeviceObject;

EXPORT
typedef STATUS (*IoCompletionCallback)(struct IoDriverRp *rp, void *context);
typedef void (*IoProcessRpCallback)(struct IoDriverRp *rp);
typedef void (*IoCancelRpCallback)(struct IoDriverRp *rp);

EXPORT
enum IoRpCode
{
    IO_RP_UNKNOWN = 0,
    //common requests
    IO_RP_READ, //read
    IO_RP_WRITE, //write
    IO_RP_IOCTL, //arbitrary i/o control
    //PnP requests
    IO_RP_START_DEVICE = 0x1000,
    
    IO_RP_ENUMERATE,
    IO_RP_GET_BUS_CONFIGURATION,
    IO_RP_GET_MMIO_MAPPING,
    IO_RP_GET_DEVICE_CONFIGURATION,
    IO_RP_SET_DEVICE_CONFIGURATION,
};

EXPORT
struct IoDriverRp
{
    struct IoSubDeviceObject *device;
    enum IoRpCode code;
    IoDriverRpFlags flags;
    void *buffer;
    uint64_t size;
    STATUS status;
    bool completed;
    union
    {
        struct
        {
            enum IoBusType type;
            struct IoSubDeviceObject *device;
            union
            {
                struct
                {
                    uint8_t bus, device, function; 
                } pci;
            }; 
        } busConfiguration;
        struct
        {
            enum IoBusType type;
            struct IoSubDeviceObject *device;
            void *memory;
            union
            {
                struct
                {
                    uint16_t segment;
                    uint8_t startBus, endBus;
                } pci;
            };
        } mmio;
        struct
        {
            enum IoBusType type;
            struct IoSubDeviceObject *device;
            uint64_t offset;
        } deviceConfiguration;
        
        
        
    } payload;
    IoCompletionCallback completionCallback;
    IoCancelRpCallback cancelCallback;
    void *completionContext;
    struct IoDriverRp *next, *previous;
    struct IoRpQueue *queue;
};

EXPORT
struct IoRpQueue
{
    IoProcessRpCallback callback;
    struct IoDriverRp *head;
    KeSpinlock queueLock;
    uint8_t busy : 1;
};

EXPORT
/**
 * @brief Create empty Request Packet
 * @param **rp Output Request Packet pointer (the memory is allocated by the function)
 * @return Status code
*/
EXTERN STATUS IoCreateRp(struct IoDriverRp **rp);

EXPORT
/**
 * @brief Create Request Packet queue
 * @param callback Callback function for processing queued RPs
 * @param **queue Output RP queue pointer (the memory is allocated by the function)
 * @return Status code
*/
EXTERN STATUS IoCreateRpQueue(IoProcessRpCallback callback, struct IoRpQueue **queue);

EXPORT
/**
 * @brief Start RP processing, that is append it to the queue
 * @param *queue Destination RP queue
 * @param *rp Input Request Packet
 * @param cancelCb Callback function required for RP cancelling; called after RP is removed from the queue. NULL if not used.
 * @return Status code
*/
EXTERN STATUS IoStartRp(struct IoRpQueue *queue, struct IoDriverRp *rp, IoCancelRpCallback cancelCb);

EXPORT
/**
 * @brief Finalize Request Packet processing
 * @param *rp Input Request Packet
 * @return Status code
 * @attention This function does not free the memory
*/
EXTERN STATUS IoFinalizeRp(struct IoDriverRp *rp);

EXPORT
/**
 * @brief Cancel pending Request Packet
 * @param *rp RP to be cancelled
 * @return Status code
 * @attention The cancel callback routine must be provided
*/
EXTERN STATUS IoCancelRp(struct IoDriverRp *rp);

#endif