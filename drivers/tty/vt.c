#include "vt.h"
#include "io/input/event.h"
#include "io/input/input.h"
#include "io/video/output.h"
#include "device.h"
#include "mm/heap.h"
#include "rtl/string.h"
#include "keymap.h"
#include "unicode/unicode.h"
#include "font.h"
#include "logging.h"

#define TTY_VT_DEFAULT_FONT_PATH "/main/SYSTEM/fonts/default8x16.psf"
#define TTY_VT_MAX_CODEPOINT_SEQ_LENGTH 8

#define TTY_HORIZONTAL_TAB_SIZE 8
#define TTY_VERTICAL_TAB_SIZE 6

static void TtyVtHandleVideoConfigChange(int handle, const union IoVideoOutput *config, void *context)
{
    //struct TtyDeviceData *info = context;


}

/**
 * @brief Process modifier keys
 * @param *info VT device data
 * @param code Key code
 * @param state Key state
 * @return True if modifier key was processed
 * @return False if processed key was not a modifier
 */
static inline bool TtyVtProcessModifierKey(struct TtyVtData *info, IoKeyCode code, bool state)
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
            if(state)
            {
                info->input.modifiers ^= TTY_VT_CAPS;
            }
            return true;
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
    struct TtyDeviceData *tty = handler->context;
    struct TtyVtData *info = &(tty->vt);
    IoKeyCode code = data->keyboard.key;
    bool state = data->keyboard.state;
    const char *key = NULL;
    size_t size = 0;

    if(unlikely(IO_EVENT_KEYBOARD != handler->type))
        return;
    
    if(TtyVtProcessModifierKey(info, code, state))
        return;
    
    if(!state)
        return;

    key = TtyDecodeKey(code, info->input.modifiers);
    if(NULL != key)
        size = RtlStrlen(key);

    if(0 != size)
        TtyProcessInput(tty, key, size);
}

static inline void TtyVtScroll(struct TtyVtData *info, uint32_t lines)
{
    if(IO_VIDEO_FRAME_BUFFER == info->output.type)
    {
        if(lines > info->output.limits.y)
            lines = info->output.limits.y;

        size_t lineSize = info->output.font->height * info->output.config.fb.config.pitch;
        RtlMemmove(info->output.config.fb.fb, (uint8_t*)info->output.config.fb.fb + lineSize * lines, 
            (info->output.limits.y - lines) * lineSize);
        
        for(size_t m = 1; m <= lines; m++)
        {
            for(size_t n = 0; n < info->output.font->height; n++)
            {
                for(size_t i = 0; i < info->output.config.fb.config.width; i++)
                {
                    for(size_t k = 0; k < info->output.bytesPerPixel; k++)
                    {
                        uint8_t *pixel = &((uint8_t*)info->output.config.fb.fb)
                            [(info->output.limits.y - m) * lineSize + n * info->output.config.fb.config.pitch + i * info->output.bytesPerPixel + k];
                        *pixel &= ~(info->output.mask >> (k * 8));
                        *pixel |= info->output.bgPixel >> (k * 8);
                    }
                }
            }
        }
    }
}

static void TtyVtHandleNewline(struct TtyVtData *info, uint32_t times)
{
    info->output.cursor.x = 0;
    if((info->output.cursor.y + times) >= info->output.limits.y)
    {
        TtyVtScroll(info, info->output.cursor.y + times - info->output.limits.y + 1);
        info->output.cursor.y = info->output.limits.y - 1;
    }
    else
    {
        info->output.cursor.y += times;
    }
}

static inline void TtyPutVtCharacter(struct TtyVtData *info, const uint32_t *code, size_t codeCount)
{
    if(info->output.limits.x == info->output.cursor.x)
        TtyVtHandleNewline(info, 1);

    if(IO_VIDEO_FRAME_BUFFER == info->output.type)
        TtyDrawGlyph(info, code, codeCount);

    ++info->output.cursor.x;
}

void TtyPutVtString(struct TtyVtData *info, const char *str, size_t len)
{
    if(unlikely(NULL == info->output.font))
        return;
    
    uint32_t code[TTY_VT_MAX_CODEPOINT_SEQ_LENGTH];
    size_t codeCount = 0;
    size_t i = 0;
    while(i < len)
    {
        if((0 != i) && (UnicodeIsGraphemeClusterStart(code[codeCount - 1], str[i])))
        {
TtyPutVtStringLast:

            switch(code[0])
            {
                case '\n':
                case '\r':
                    TtyVtHandleNewline(info, 1);
                    break;
                case '\v':
                    TtyVtHandleNewline(info, TTY_VERTICAL_TAB_SIZE);
                    break;
                case '\t':
                    for(size_t i = 0; i < TTY_HORIZONTAL_TAB_SIZE; i++)
                    {
                        uint32_t c = ' ';
                        TtyPutVtCharacter(info, &c, 1);
                    }
                    break;
                case '\b':
                    if((info->output.cursor.x > 0) || (info->output.cursor.y > 0))
                    {
                        uint32_t c = ' ';
                        if(info->output.cursor.x > 0)
                        {
                            --info->output.cursor.x;
                            TtyPutVtCharacter(info, &c, 1);
                            --info->output.cursor.x;
                        }
                        else //x == 0 and y > 0
                        {
                            --info->output.cursor.y;
                            info->output.cursor.x = info->output.limits.x - 1;
                            TtyPutVtCharacter(info, &c, 1);
                            info->output.cursor.x = info->output.limits.x - 1;
                        }
                    }
                    break;
                default:
                    TtyPutVtCharacter(info, code, codeCount);
                    break;
            }

            if(i == len)
                break;
            codeCount = 0;
        }

        size_t byteCount = Utf8ByteCount(str[i]);
        if((i + byteCount) > len)
            goto TtyPutVtStringEnd;

        switch(byteCount)
        {
            case 2:
                code[codeCount] = ((str[i] & 0x1F) << 6) | (str[i + 1] & 0x3F);
                break;
            case 3:
                code[codeCount] = ((str[i] & 0x0F) << 12) | ((str[i + 1] & 0x3F) << 6) | (str[i + 2] & 0x3F);
                break;
            case 4:
                code[codeCount] = ((str[i] & 0x07) << 18) | ((str[i + 1] & 0x3F) << 12) | ((str[i + 2] & 0x3F) << 6) | (str[i + 3] & 0x3F);
                break;
            case 1:
            default:
                code[codeCount] = str[i];
                break;
        }

        i += byteCount;
        ++codeCount;

        if(len == i)
            goto TtyPutVtStringLast;
    }

TtyPutVtStringEnd:
    if(IO_VIDEO_FRAME_BUFFER == info->output.type)
        IoDrawVideo(info->output.handle);
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

        //do not support non-integer number of bytes per pixel and more than 64 bits per pixel, as it is a pain to handle
        if((info->vt.output.config.fb.config.bitsPerPixel % 8) || (info->vt.output.config.fb.config.bitsPerPixel > 64))
            status = NOT_SUPPORTED;
        else
            info->vt.output.bytesPerPixel = info->vt.output.config.fb.config.bitsPerPixel / 8;

        if(OK != status)
        {
            MmFreeKernelHeap(info);
            return status;
        }
    }

    struct IoEventHandler evHandler;
    if(params->request.createVt.inputEvent > -1)
    {
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
    }

    status = TtyCreateDevice(drv, TTY_TYPE_VT, info);
    if(OK != status)
    {
        if(params->request.createVt.outputDisplay > -1)
            IoRemoveVideoOutputConfigChangeHandler(params->request.createVt.outputDisplay, TtyVtHandleVideoConfigChange, info);
        if(params->request.createVt.inputEvent > -1)
            IoUnregisterEventHandler(&evHandler);
        MmFreeKernelHeap(info);
        return status;
    }

    info->type = TTY_TYPE_VT;
    info->vt.input.handle = params->request.createVt.inputEvent;
    info->vt.output.handle = params->request.createVt.outputDisplay;

    RtlStrcpy(params->request.createVt.name, info->name);

    if(OK != TtyLoadFont(&info->vt, TTY_VT_DEFAULT_FONT_PATH))
    {
        LOG(SYSLOG_ERROR, "Failed to load default font from %s for VT %s", TTY_VT_DEFAULT_FONT_PATH, info->name);
    }

    //set default color to black on white
    TtyVtSetColor(&info->vt, (TtyColor){255, 255, 255}, (TtyColor){0, 0, 0});

    info->vt.output.mask = 0;
    for(size_t i = 0; i < 3; i++)
    {
        info->vt.output.mask |= (((uint64_t)1 << info->vt.output.config.fb.config.mask.color[i].size) - (uint64_t)1) 
            << info->vt.output.config.fb.config.mask.color[i].position;
    }

    return OK;
}

void TtyVtSetColor(struct TtyVtData *vt, TtyColor fg, TtyColor bg)
{
    vt->output.fgPixel = 0;
    vt->output.bgPixel = 0;

    for(size_t i = 0; i < 3; i++)
    {
        vt->output.fg[i] = fg[i];
        vt->output.bg[i] = bg[i];

        vt->output.fgPixel |= 
            ((uint64_t)fg[i] >> (8 - vt->output.config.fb.config.mask.color[i].size)) << vt->output.config.fb.config.mask.color[i].position;
        vt->output.bgPixel |= 
            ((uint64_t)bg[i] >> (8 - vt->output.config.fb.config.mask.color[i].size)) << vt->output.config.fb.config.mask.color[i].position;
    }
}