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
#include "hal/interrupt.h"
#include "io/fs/vfs.h"
#include "res.h"
struct IoRp;
struct IoRpQueue;
struct IoDeviceObject;

typedef STATUS (*IoCompletionCallback)(struct IoRp *rp, void *context);
typedef void (*IoProcessRpCallback)(struct IoRp *rp);
typedef void (*IoCancelRpCallback)(struct IoRp *rp);

enum IoRpCode
{
    IO_RP_UNKNOWN = 0, /**< Unknown request, do not use */
    //common requests
    IO_RP_READ, /**< Read file */
    IO_RP_WRITE, /**< Write file */
    IO_RP_OPEN, /**< Open file */
    IO_RP_IOCTL, /**< Driver-defined I/O control */
    //PnP requests
    IO_RP_START_DEVICE = 0x1000,
    
    IO_RP_GET_DEVICE_ID, /**< Get device ID and compatible IDs */
    IO_RP_GET_DEVICE_TEXT, /**< Get user-friendly device name */
    
    IO_RP_ENUMERATE, /**< Enumerate children of the device */
    IO_RP_GET_DEVICE_LOCATION, /**< Get device location on the bus */
    IO_RP_GET_DEVICE_RESOURCES, /**< Get device resources: IRQs, MMIOs, ports, etc. */
    IO_RP_GET_CONFIG_SPACE, /**< Read device configuration space */
    IO_RP_SET_CONFIG_SPACE, /**< Write device configuration space */

    //device type specific control requests
    IO_RP_STORAGE_CONTROL = 0x2000,

};

struct IoRp
{
    struct IoDeviceObject *device;
    struct IoVfsNode *vfsNode;
    enum IoRpCode code;
    IoRpFlags flags;
    void *buffer;
    uint64_t size;
    STATUS status;
    bool completed;
    union
    {
        struct
        {
            uint32_t count; /**< Count of resource entries */
            struct IoDeviceResource *res; /**< Contiguous list of \a count resources */
        } resource;

        /**
         * @brief \a IO_RP_GET_DEVICE_LOCATION
        */
        struct 
        {
            enum IoBusType type;
            union IoBusId id;
        } location;
        

        /**
         * @brief \a IO_RP_GET_CONFIG_SPACE and \a IO_RP_SET_CONFIG_SPACE
        */
        struct
        {
            uint64_t offset;
        } configSpace;

        /**
         * @brief \a IO_RP_READ and \a IO_RP_WRITE
        */
        struct 
        {
            uint64_t offset;
            struct IoMemoryDescriptor *memory;
            void *systemBuffer;
        } readWrite;

        /**
         * @brief \a IO_RP_GET_DEVICE_ID
        */
        struct
        {
            char *mainId;
            char **compatibleId;
        } deviceId;

        /**
         * @brief \a IO_RP_GET_DEVICE_TEXT
        */
        char *text;

        /**
         * @brief Payload for device type specific requests
        */
        struct
        {
            uint32_t code;
            void *data;
        } deviceControl;
    } payload;
    IoCompletionCallback completionCallback;
    IoCancelRpCallback cancelCallback;
    void *completionContext;
    struct IoRp *next, *previous;
    struct IoRpQueue *queue;
    struct IoDeviceObject *currentPosition;
};

struct IoRpQueue
{
    IoProcessRpCallback callback;
    struct IoRp *head;
    KeSpinlock queueLock;
    uint8_t busy : 1;
};

/**
 * @brief Create empty Request Packet
 * @param **rp Output Request Packet pointer (the memory is allocated by the function)
 * @return Status code
*/
extern STATUS IoCreateRp(struct IoRp **rp);

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
extern STATUS IoStartRp(struct IoRpQueue *queue, struct IoRp *rp, IoCancelRpCallback cancelCb);

/**
 * @brief Finalize Request Packet processing
 * @param *rp Input Request Packet
 * @return Status code
 * @attention This function does not free the memory
*/
extern STATUS IoFinalizeRp(struct IoRp *rp);

/**
 * @brief Cancel pending Request Packet
 * @param *rp RP to be cancelled
 * @return Status code
 * @attention The cancel callback routine must be provided
*/
extern STATUS IoCancelRp(struct IoRp *rp);

/**
 * @brief Get current Request Packet position in device stack
 * @param *rp Request Packet pointer
 * @return Current subdevice object
*/
extern struct IoDeviceObject* IoGetCurrentRpPosition(struct IoRp *rp);


#ifdef __cplusplus
}
#endif

#endif