#include "vt.h"
#include "io/input/event.h"
#include "io/input/input.h"
#include "io/video/output.h"
#include "device.h"
#include "mm/heap.h"
#include "rtl/string.h"
#include "keymap.h"

static void TtyVtHandleVideoConfigChange(int handle, const union IoVideoOutput *config, void *context)
{
    struct TtyDeviceData *info = context;


}

/**
 * @brief Process modifier keys
 * @param *info VT device data
 * @param code Key code
 * @param state Key state
 * @return True if modifier key was processed
 * @return False if processed key was not a modifier
 */
static inline bool TtyProcessModifierKey(struct TtyVtData *info, IoKeyCode code, bool state)
{
    uint32_t modifier = 0;
    switch(code)
    {
        case IO_KEY_LEFT_CTRL:
            modifier = TTY_VT_LEFT_CTRL;
            break;
        case IO_KEY_RIGHT_CTRL:
            modifier = TTY_VT_RIGHT_CTRL;
            break;
        case IO_KEY_LEFT_ALT:
            modifier = TTY_VT_LEFT_ALT;
            break;
        case IO_KEY_RIGHT_ALT:
            modifier = TTY_VT_RIGHT_ALT;
            break;
        case IO_KEY_LEFT_SHIFT:
            modifier = TTY_VT_LEFT_SHIFT;
            break;
        case IO_KEY_RIGHT_SHIFT:
            modifier = TTY_VT_RIGHT_SHIFT;
            break;
        case IO_KEY_CAPS_LOCK:
            modifier = TTY_VT_CAPS;
            break;
        case IO_KEY_LEFT_SYSTEM:
            modifier = TTY_VT_LEFT_SYSTEM;
            break;
        case IO_KEY_RIGHT_SYSTEM:
            modifier = TTY_VT_RIGHT_SYSTEM;
            break;
        case IO_KEY_MENU:
            modifier = TTY_VT_MENU;
            break;
        default:
            return false;
            break;
    }

    if(state)
        info->input.modifiers |= modifier;
    else
        info->input.modifiers &= ~modifier;

    return true;
}

static void TtyProcessVtInput(const struct IoEventHandler *handler, const union IoEventData *data)
{
    struct TtyVtData *info = &((struct TtyDeviceData*)handler->context)->vt;
    IoKeyCode code = data->keyboard.key;
    bool state = data->keyboard.state;
    const char *key = NULL;

    if(unlikely(IO_EVENT_KEYBOARD != handler->type))
        return;
    
    if(TtyProcessModifierKey(info, code, state))
        return;
    
    key = TtyDecodeKey(code, info->input.modifiers);
    if((NULL != key) && ('\0' != *key))
    {
        size_t len = GET_UTF8_BYTE_COUNT(*key);
    }
    
}

STATUS TtyCreateVt(struct ExDriverObject *drv, struct TtyParameters *params)
{
    STATUS status = OK;

    struct TtyDeviceData *info = MmAllocateKernelHeapZeroed(sizeof(*info));
    if(NULL == info)
        return OUT_OF_RESOURCES;

    if(params->request.createVt.outputDisplay > -1)
    {
        status = IoGetVideoOutput(params->request.createVt.outputDisplay, TtyVtHandleVideoConfigChange, 
            info, &info->vt.output.config, &info->vt.output.type);
        if(OK != status)
        {
            MmFreeKernelHeap(info);
            return status;
        }
    }

    struct IoEventHandler evHandler;
    evHandler.type = IO_EVENT_KEYBOARD;
    evHandler.aggregate = false;
    evHandler.handle = params->request.createVt.inputEvent;
    evHandler.context = info;
    evHandler.event = TtyProcessVtInput;

    status = IoRegisterEventHandler(&evHandler);
    if(OK != status)
    {
        IoRemoveVideoOutputConfigChangeHandler(params->request.createVt.outputDisplay, TtyVtHandleVideoConfigChange, info);
        MmFreeKernelHeap(info);
        return status;
    }

    status = TtyCreateDevice(drv, TTY_TYPE_VT, info);
    if(OK != status)
    {
        IoRemoveVideoOutputConfigChangeHandler(params->request.createVt.outputDisplay, TtyVtHandleVideoConfigChange, info);
        IoUnregisterEventHandler(&evHandler);
        MmFreeKernelHeap(info);
        return status;
    }

    info->vt.input.handle = params->request.createVt.inputEvent;
    info->vt.output.handle = params->request.createVt.outputDisplay;

    RtlStrcpy(params->request.createVt.name, info->name);

    return OK;
}