#ifndef KERNEL_RP_H_
#define KERNEL_RP_H_

#include <stdint.h>
#include "defines.h"
#include "bus.h"
#include "ob/ob.h"

EXPORT_API

struct IoRp;
struct IoRpQueue;
struct IoDeviceObject;
struct IoVfsNode;
struct IoDeviceResource;
struct KeTaskControlBlock;


typedef STATUS (*IoRpCompletionCallback)(struct IoRp *rp, void *context);
typedef void (*IoProcessRpCallback)(struct IoRp *rp);
typedef void (*IoRpCancelCallback)(struct IoRp *rp);


typedef uint32_t IoRpFlags; /**< Request Packet flags */
#define IO_RP_FLAG_EOF 0x1 /**< Read incomplete, end of file encountered - \a status is set to \a OK, chech \a size field */


enum IoRpCode
{
    IO_RP_UNKNOWN = 0, /**< Unknown request, do not use */
    //common requests
    IO_RP_READ, /**< Read file */
    IO_RP_WRITE, /**< Write file */
    IO_RP_OPEN, /**< Open file */
    IO_RP_CLOSE, /**< Close file */
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
    IO_RP_FILESYSTEM_CONTROL,
    IO_RP_DISK_CONTROL,
};


struct IoRp
{
    struct ObObjectHeader objectHeader;
    struct IoDeviceObject *device; /**< Target device for this request */
    struct IoVfsNode *vfsNode; /**< VFS node associated with this request */
    enum IoRpCode code; /**< Request code */
    IoRpFlags flags; /**< Request flags */
    void *systemBuffer; /**< System buffer in kernel space */
    void *userBuffer; /**< User buffer in user space */
    uint64_t size; /**< Request data size */
    STATUS status; /**< Request status */
    bool pending; /**< Pending flag */
    struct KeTaskControlBlock *task; /**< Associated task */
    union
    {
        /**
         * @brief \a IO_RP_GET_DEVICE_RESOURCES
        */
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
            void *buffer;
        } configSpace;

        /**
         * @brief \a IO_RP_READ and \a IO_RP_WRITE
        */
        struct 
        {
            uint64_t offset; /**< File offset */
            struct MmMemoryDescriptor *memory; /**< Memory descriptors for direct I/O */
            void *systemBuffer; /**< Buffer for buffered I/O */
        } read, write;

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

        /**
         * @brief \a IO_RP_IOCTL
         */
        struct
        {
            uint32_t code;
            void *data;
        } ioctl;
        
        

    } payload;
    IoRpCompletionCallback completionCallback;
    IoRpCancelCallback cancelCallback;
    void *completionContext;
    struct IoRp *next, *previous;
    struct IoRpQueue *queue;
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
 * @return Created Request Packet pointer or NULL on failure
*/
struct IoRp *IoCreateRp(void);


/**
 * @brief Free Request Packet
 * @param *rp Request Packet pointer obtained from \a IoCreateRp()
*/
void IoFreeRp(struct IoRp *rp);


/**
 * @brief Create Request Packet queue
 * @param callback Callback function for processing queued RPs
 * @param **queue Output RP queue pointer (the memory is allocated by the function)
 * @return Status code
*/
STATUS IoCreateRpQueue(IoProcessRpCallback callback, struct IoRpQueue **queue);

/**
 * @brief Destroy Request Packet queue
 * @param *queue RP queue to be destroyed
 * @return Status code
 * @attention It is the caller's responsibility to make sure that the queue is empty and unused
 */
STATUS IoDestroyRpQueue(struct IoRpQueue *queue);


/**
 * @brief Start RP processing, that is append it to the queue
 * @param *queue Destination RP queue
 * @param *rp Input Request Packet
 * @param cancelCb Callback function required for RP cancelling; called after RP is removed from the queue. NULL if not used.
 * @return Status code
*/
STATUS IoStartRp(struct IoRpQueue *queue, struct IoRp *rp, IoRpCancelCallback cancelCb);


/**
 * @brief Finalize Request Packet processing
 * @param *rp Input Request Packet
 * @return Status code
 * @attention This function frees the memory when there is a completion callback registered
*/
STATUS IoFinalizeRp(struct IoRp *rp);


/**
 * @brief Cancel pending Request Packet
 * @param *rp RP to be cancelled
 * @return Status code
 * @attention The cancel callback routine must be provided
*/
STATUS IoCancelRp(struct IoRp *rp);


/**
 * @brief Get current Request Packet position in device stack
 * @param *rp Request Packet pointer
 * @return Current subdevice object
*/
struct IoDeviceObject* IoGetCurrentRpPosition(struct IoRp *rp);


/**
 * @brief Mark Request Packet as pending
 * 
 * This routine marks RP as pending in case when the driver is not able to finalize RP immediately,
 * e.g. when it needs to wait for an IRQ. In such case, the driver should mark the RP as pending and return
 * from the RP dispatch routine. The driver should not call this routine when passing RP to another driver.
 * @param *rp Request Packet
*/
void IoMarkRpPending(struct IoRp *rp);


/**
 * @brief Wait for RP completion
 * 
 * This function blocks calling task and waits for RP completion.
 * This function returns when the driver calls \a IoFinalizeRp().
 * @param *rp Request Packet
 * @warning This function must not be called when \a IoSendRp() was not successful.
*/
void IoWaitForRpCompletion(struct IoRp *rp);


/**
 * @brief Clone exisiting RP
 * @param *rp RP to clone
 * @return New cloned RP or NULL on failure
 */
struct IoRp *IoCloneRp(struct IoRp *rp);

END_EXPORT_API

/**
 * @brief Initialize RP slab cache
 * @return Status code
*/
INTERNAL STATUS IoInitializeRpCache(void);

#endif