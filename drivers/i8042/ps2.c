#include "ps2.h"
#include "device.h"
#include "io/dev/rp.h"
#include "ke/core/dpc.h"
#include "ke/core/mutex.h"
#include "kbd.h"
#include "io/input/kbd.h"
#include "rtl/string.h"
#include "io/dev/dev.h"

#define PS2_RETRIES 5 /**< Number of retries before failing */
#define PS2_BUFFER_SIZE 64 /**< Size of non-command PS/2 buffer, must be 2^k */

#define PS2_COMMAND_ENABLE_SCANNING 0xF4
#define PS2_COMMAND_DISABLE_SCANNING 0xF5
#define PS2_COMMAND_RESET 0xFF
#define PS2_COMMAND_IDENTIFY 0xF2

#define PS2_RESPONSE_ERROR_1 0x00
#define PS2_RESPONSE_ERROR_2 0xFF
#define PS2_RESPONSE_ECHO 0xEE
#define PS2_RESPONSE_ACK 0xFA
#define PS2_RESPONSE_RESEND 0xFE
#define PS2_RESPONSE_TEST_PASSED 0xAA

static struct Ps2State
{
    uint8_t port;
    struct
    {
        uint8_t data[PS2_BUFFER_SIZE];
        uint8_t head, tail;
        bool full;
        bool dpcPending;
        KeSpinlock lock;
    } buffer;
    struct
    {
        uint8_t data[4];
        uint8_t size;
        uint8_t count;
        uint8_t retries;
        bool inProgress;
        struct IoRp *rp;
    } command;
} 
Ps2State[2] = {{.port = PORT_FIRST, .buffer.head = 0, .buffer.tail = 0, .buffer.full = false, .buffer.dpcPending = false,
     .buffer.lock = KeSpinlockInitializer, .command.inProgress = false}, 
    {.port = PORT_SECOND, .buffer.head = 0, .buffer.tail = 0, .buffer.full = false, .buffer.dpcPending = false,
     .buffer.lock = KeSpinlockInitializer, .command.inProgress = false}};

static bool Ps2WriteAndRead(struct I8042Peripheral *info, uint8_t command, uint8_t *output, uint16_t count)
{
    bool status = false;
    for(uint8_t i = 0; i < PS2_RETRIES; i++)
    {
        if(I8042WriteToPeripheral(info->port, command))
        {
            if(I8042ReadFromPeripheral(output, count))
            {
                if(PS2_RESPONSE_ACK == output[0])
                {
                    status = true;
                    break;
                }
            }
        }
    }
    return status;   
}

bool Ps2WriteMultiple(struct I8042Peripheral *info, uint8_t *data, uint16_t count)
{
    uint8_t buffer;
    bool status = false;
    for(uint16_t i = 0; i < count; i++)
    {
        for(uint8_t r = 0; r < PS2_RETRIES; r++)
        {
            if(I8042WriteToPeripheral(info->port, data[i]))
            {
                if(I8042ReadFromPeripheral(&buffer, 1))
                {
                    if(PS2_RESPONSE_ACK == buffer)
                    {
                        status = true;
                        break;
                    }
                }
            }
        }
    }
    return status;   
}

static void Ps2ProcessDpc(void *context)
{
    uint16_t key[PS2_BUFFER_SIZE];
    uint8_t keyCount = 0;
    struct Ps2State *s = context;
    if(s->command.inProgress)
    {
        if((s->command.count == s->command.size) || (PS2_RETRIES == s->command.retries))
        {
            s->command.inProgress = false;
            s->command.count = 0;
            s->command.size = 0;
            s->command.retries = 0;
            if(NULL != s->command.rp)
            {
                if(PS2_RETRIES == s->command.retries)
                    s->command.rp->status = TIMEOUT;
                else
                    s->command.rp->status = OK;
                IoFinalizeRp(s->command.rp);
            }
        }
        else
        {
            uint8_t retries = 0;
            while(false == I8042WriteToPeripheral(s->port, s->command.data[s->command.count]))
            {
                if(PS2_RETRIES == retries)
                {
                    s->command.inProgress = false;
                    s->command.count = 0;
                    s->command.size = 0;
                    s->command.retries = 0;
                    if(NULL != s->command.rp)
                    {
                        s->command.rp->status = TIMEOUT;
                        IoFinalizeRp(s->command.rp);
                    }
                    break;
                }
                retries++;
            }
        }
    }

    PRIO prio = KeAcquireSpinlock(&(s->buffer.lock));
    if(s->buffer.full || (s->buffer.head != s->buffer.tail))
    {
        uint8_t buffer[16];
        uint8_t index = 0;
        uint8_t head = s->buffer.head;
        uint8_t keyHead = head;
        do
        {
            key[keyCount] = Ps2KeyboardParse(s->buffer.data[head], buffer, &index);
            head++;
            head &= (PS2_BUFFER_SIZE - 1);

            if((PS2_KEY_INCOMPLETE != key[keyCount]) && (0 != (key[keyCount] & PS2_KEY_VALUE_MASK)))
            {
                keyHead = head;
                keyCount++;
            }
        }
        while(head != s->buffer.tail);

        s->buffer.head = keyHead;
    }
    s->buffer.dpcPending = false;
    KeReleaseSpinlock(&(s->buffer.lock), prio);

    for(uint8_t i = 0; i < keyCount; i++)
    {
        if(key[i] & PS2_KEY_PULSE_BIT)
        {
            IoKeyboardReportKey(I8042ControllerInfo.port[s->port].device->handle, key[i] & PS2_KEY_VALUE_MASK, true);
            IoKeyboardReportKey(I8042ControllerInfo.port[s->port].device->handle, key[i] & PS2_KEY_VALUE_MASK, false);
        }
        else
            IoKeyboardReportKey(I8042ControllerInfo.port[s->port].device->handle, key[i] & PS2_KEY_VALUE_MASK, 
                (key[i] & PS2_KEY_STATE_BIT) ? true : false);
    }
}

void Ps2HandleIrq(uint8_t port, uint8_t data)
{
    //TODO: handle timeouts
    if(Ps2State[port].command.inProgress)
    {
        if(PS2_RESPONSE_ACK == data)
        {
            Ps2State[port].command.count++;
            Ps2State[port].command.retries = 0;
            if(OK == KeRegisterDpc(KE_DPC_PRIORITY_NORMAL, Ps2ProcessDpc, &Ps2State[port]))
                Ps2State[port].buffer.dpcPending = true;
            return;
        }
        else if(PS2_RESPONSE_RESEND == data)
        {
            Ps2State[port].command.retries++;
            if(OK == KeRegisterDpc(KE_DPC_PRIORITY_NORMAL, Ps2ProcessDpc, &Ps2State[port]))
                Ps2State[port].buffer.dpcPending = true;
            return;
        }
    }
    
    PRIO prio = KeAcquireSpinlock(&(Ps2State[port].buffer.lock));
    if(!Ps2State[port].buffer.full)
    {
        Ps2State[port].buffer.data[Ps2State[port].buffer.tail++] = data;
        Ps2State[port].buffer.tail &= (PS2_BUFFER_SIZE - 1);
        if(Ps2State[port].buffer.tail == Ps2State[port].buffer.head)
            Ps2State[port].buffer.full = true;

        if(!Ps2State[port].buffer.dpcPending)
        {
            if(PS2_KEYBOARD == I8042ControllerInfo.port[port].device->type)
            {
                //check if there is any unprocessed key
                uint8_t head = Ps2State[port].buffer.head;
                do
                {
                    uint8_t b = Ps2State[port].buffer.data[head];
                    
                    //a "normal" byte is found, which potentially might be processed
                    if((0xE0 != b) && (0xF0 != b) && (0xE1 != b))
                    {
                        if(OK == KeRegisterDpc(KE_DPC_PRIORITY_NORMAL, Ps2ProcessDpc, &Ps2State[port]))
                            Ps2State[port].buffer.dpcPending = true;
                        break;    
                    }

                    head++;
                    head &= (PS2_BUFFER_SIZE - 1);
                }
                while(head != Ps2State[port].buffer.tail);
            }
        }
    }
    barrier();
    KeReleaseSpinlock(&(Ps2State[port].buffer.lock), prio);
}

bool Ps2ProbePort(struct I8042Peripheral *info)
{
    bool status = true;
    uint8_t buffer[4];

    I8042SetInterruptsEnabled(info->port, false);

    Ps2WriteAndRead(info, PS2_COMMAND_RESET, buffer, 4);
    if(PS2_RESPONSE_TEST_PASSED == buffer[1])
    {
        if(Ps2SetScanningEnabled(info, false))
        {
            Ps2WriteAndRead(info, PS2_COMMAND_IDENTIFY, buffer, 3);
            //don't care if it timed out - device may send less than 3 bytes
            if(PS2_RESPONSE_ACK == buffer[0])
            {
                switch(buffer[1])
                {
                    case 0x00:
                    case 0x03:
                    case 0x04:
                        info->type = PS2_MOUSE;
                        break;
                    case 0xAB:
                        switch(buffer[2])
                        {
                            case 0x83:
                            case 0x84:
                            case 0xC1:
                            case 0x54:
                            case 0x85:
                            case 0x86:
                            case 0x90:
                            case 0x91: 
                            case 0x92:
                                info->type = PS2_KEYBOARD;
                                info->device->type = IO_DEVICE_TYPE_KEYBOARD;
                                break;
                            default:
                                info->type = PS2_UNKNOWN;
                        }
                        break;
                    case 0xAC:
                        switch(buffer[2])
                        {
                            case 0xA1:
                                info->type = PS2_KEYBOARD;
                                info->device->type = IO_DEVICE_TYPE_KEYBOARD;
                                break;
                            default:
                                info->type = PS2_UNKNOWN;
                                break;
                        }
                        break;
                    default:
                        info->type = PS2_UNKNOWN;
                        break;
                }
            }

            if(PS2_KEYBOARD == info->type)
            {
                status = Ps2KeyboardSetUp(info);
                if(status)
                {
                    status = Ps2SetScanningEnabled(info, true);
                }
            }
        }
    }

    I8042SetInterruptsEnabled(info->port, true);

    return status;
}

bool Ps2SetScanningEnabled(struct I8042Peripheral *info, bool state)
{
    uint8_t response;
    return Ps2WriteAndRead(info, state ? PS2_COMMAND_ENABLE_SCANNING : PS2_COMMAND_DISABLE_SCANNING, &response, 1);
}

void Ps2SendCommand(struct I8042Peripheral *info, const void *data, uint8_t size, struct IoRp *rp)
{
    struct Ps2State *s = &(Ps2State[info->port]);
    s->command.size = size;
    s->command.inProgress = true;
    s->command.retries = 0;
    s->command.count = 0;
    s->command.rp = rp;
    RtlMemcpy(s->command.data, data, size);
    uint8_t retries = 0;
    while(false == I8042WriteToPeripheral(s->port, s->command.data[0]))
    {
        if(PS2_RETRIES == retries)
        {
            s->command.inProgress = false;
            s->command.count = 0;
            s->command.size = 0;
            s->command.retries = 0;
            if(NULL != s->command.rp)
            {
                s->command.rp->status = TIMEOUT;
                IoFinalizeRp(s->command.rp);
            }
            break;
        }
        retries++;
    }
}