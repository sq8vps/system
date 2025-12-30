#include "input.h"
#include "ke/core/mutex.h"
#include "event.h"
#include "ob/ob.h"
#include "io/dev/dev.h"
#include "rtl/string.h"

#include "io/log/syslog.h"

/**
 * @brief Maximum number of registered input devices
 */
#define IO_INPUT_MAX 128

struct IoInputDevice
{
    const struct IoDeviceObject *dev;
    int handle;
    enum IoEventType type;
    struct IoEventHandler *event;
};

static struct IoInputDevice IoInputDeviceList[IO_INPUT_MAX] 
    = {[0 ... IO_INPUT_MAX - 1] = {.dev = NULL, .handle = -1, .event = NULL}};

static struct IoEventHandler *IoAggregatedEventList[IO_EVENT_TYPE_COUNT] = {[0 ... IO_EVENT_TYPE_COUNT - 1] = NULL};
static KeSpinlock IoEventListsLock = KeSpinlockInitializer;

STATUS IoRegisterInputDevice(const struct IoDeviceObject *dev, int *handle)
{
    *handle = -1;

    PRIO prio = KeAcquireDpcLevelSpinlock(&IoEventListsLock);
    for(size_t i = 0; i < IO_INPUT_MAX; i++)
    {
        if(IoInputDeviceList[i].handle < 0)
        {
            IoInputDeviceList[i].handle = i;
            IoInputDeviceList[i].dev = dev;
            switch(dev->type)
            {
                case IO_DEVICE_TYPE_KEYBOARD:
                    IoInputDeviceList[i].type = IO_EVENT_KEYBOARD;
                    break;
                default:
                    IoInputDeviceList[i].type = IO_EVENT_UNKNOWN;
                    break;
            }

            *handle = i;
            break;
        }
    }
    KeReleaseSpinlock(&IoEventListsLock, prio);

    if(-1 == *handle)
        return OUT_OF_RESOURCES;
    else
        return OK;
}

STATUS IoRegisterEventHandler(const struct IoEventHandler *handler)
{
    STATUS status = OK;

    if(!handler->aggregate && ((handler->handle < 0) || (handler->handle >= IO_INPUT_MAX)))
        return NOT_FOUND;

    if(handler->aggregate && ((handler->type < 0) || (handler->type >= IO_EVENT_TYPE_COUNT)))
        return NOT_FOUND;
    
    if(NULL == handler->event)
        return BAD_PARAMETER;

    struct IoEventHandler *h = ObCreateKernelObject(OB_EVENT);
    if(NULL == h)
        return OUT_OF_RESOURCES;

    h->aggregate = handler->aggregate;
    h->context = handler->context;
    h->event = handler->event;
    h->handle = handler->handle;
    h->type = handler->type;
    h->next = NULL;

    PRIO prio = KeAcquireDpcLevelSpinlock(&IoEventListsLock);

    if(!handler->aggregate)
    {
        if(IoInputDeviceList[h->handle].handle < 0)
        {
            status = NOT_FOUND;
        }
        else
        {
            if(h->type != IoInputDeviceList[h->handle].type)
            {
                KeReleaseSpinlock(&IoEventListsLock, prio);
                ObDestroyObject(h);
                return BAD_TYPE;
            }

            if(NULL == IoInputDeviceList[h->handle].event)
                IoInputDeviceList[h->handle].event = h;
            else
            {
                struct IoEventHandler *t = IoInputDeviceList[handler->handle].event;
                while(NULL != t->next)
                    t = t->next;

                t->next = h;
            }
        }
    }
    else
    {
        if(NULL == IoAggregatedEventList[h->type])
            IoAggregatedEventList[h->type] = h;
        else
        {
            struct IoEventHandler *t = IoAggregatedEventList[h->type];
            while(NULL != t->next)
                t = t->next;

            t->next = h;
        }
    }
    KeReleaseSpinlock(&IoEventListsLock, prio);

    return status;
}

STATUS IoUnregisterEventHandler(const struct IoEventHandler *handler)
{
    UNUSED(handler);
    //TODO: implement!!!
    return NOT_IMPLEMENTED;   
}

STATUS IoReportEvent(int handle, const union IoEventData *data)
{
    if((handle < 0) || (handle >= IO_INPUT_MAX))
        return NOT_FOUND;

    if(IoInputDeviceList[handle].handle < 0)
        return NOT_FOUND;

    PRIO prio = KeAcquireDpcLevelSpinlock(&IoEventListsLock);
    //notify aggregated event recipients
    const struct IoEventHandler *h = IoAggregatedEventList[IoInputDeviceList[handle].type];
    while(NULL != h)
    {
        if(NULL != h->event)
        {
            h->event(h, data);
        }
        h = h->next;
    }
    
    //notify device-oriented event recipients
    h = IoInputDeviceList[handle].event;
    while(NULL != h)
    {
        if(NULL != h->event)
        {
            h->event(h, data);
        }
        h = h->next;
    }
    KeReleaseSpinlock(&IoEventListsLock, prio);

    return OK;
}