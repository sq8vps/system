//This header file is generated automatically
#ifndef EXPORTED___API__IO_DEV_RP_H_
#define EXPORTED___API__IO_DEV_RP_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include <stdint.h>
#include "defines.h"
#include "dev.h"
#include "typedefs.h"
#include "ke/core/mutex.h"
#include "types.h"
struct IoDriverRp;
struct IoRpQueue;
struct IoSubDeviceObject;

typedef STATUS (*IoCompletionCallback)(struct IoDriverRp *rp, void *context);
typedef void (*IoProcessRpCallback)(struct IoDriverRp *rp);
typedef void (*IoCancelRpCallback)(struct IoDriverRp *rp);

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

struct IoRpQueue
{
    IoProcessRpCallback callback;
    struct IoDriverRp *head;
    KeSpinlock queueLock;
    uint8_t busy : 1;
};

/**
 * @brief Create empty Request Packet
 * @param **rp Output Request Packet pointer (the memory is allocated by the function)
 * @return Status code
*/
extern STATUS IoCreateRp(struct IoDriverRp **rp);

/**
 * @brief Create Request Packet queue
 * @param callback Callback function for processing queued RPs
 * @param **queue Output RP queue pointer (the memory is allocated by the function)
 * @return Status code
*/
extern STATUS IoCreateRpQueue(IoProcessRpCallback callback, struct IoRpQueue **queue);

/**
 * @brief Start RP processing, that is append it to the queue
 * @param *queue Destination RP queue
 * @param *rp Input Request Packet
 * @param cancelCb Callback function required for RP cancelling; called after RP is removed from the queue. NULL if not used.
 * @return Status code
*/
extern STATUS IoStartRp(struct IoRpQueue *queue, struct IoDriverRp *rp, IoCancelRpCallback cancelCb);

/**
 * @brief Finalize Request Packet processing
 * @param *rp Input Request Packet
 * @return Status code
 * @attention This function does not free the memory
*/
extern STATUS IoFinalizeRp(struct IoDriverRp *rp);

/**
 * @brief Cancel pending Request Packet
 * @param *rp RP to be cancelled
 * @return Status code
 * @attention The cancel callback routine must be provided
*/
extern STATUS IoCancelRp(struct IoDriverRp *rp);


#ifdef __cplusplus
}
#endif

#endif