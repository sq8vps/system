#include "ps2.h"
#include "device.h"

#define PS2_RETRIES 5 /**< Number of retries before failing */

#define PS2_COMMAND_ENABLE_SCANNING 0xF4
#define PS2_COMMAND_DISABLE_SCANNING 0xF5
#define PS2_COMMAND_RESET 0xFF
#define PS2_COMMAND_IDENTIFY 0xF2
#define PS2_COMMAND_SET_SCAN_CODE_SET 0xF0

#define PS2_RESPONSE_ACK 0xFA
#define PS2_RESPONSE_RESEND 0xFE
#define PS2_RESPONSE_TEST_PASSED 0xAA

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

static bool Ps2WriteMultiple(struct I8042Peripheral *info, uint8_t *data, uint16_t count)
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

bool Ps2ProbePort(struct I8042Peripheral *info)
{
    uint8_t buffer[4];
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
                buffer[0] = PS2_COMMAND_SET_SCAN_CODE_SET;
                buffer[1] = 2;
                if(Ps2WriteMultiple(info, buffer, 2))
                {
                    if(Ps2SetScanningEnabled(info, true))
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

bool Ps2SetScanningEnabled(struct I8042Peripheral *info, bool state)
{
    uint8_t response;
    return Ps2WriteAndRead(info, state ? PS2_COMMAND_ENABLE_SCANNING : PS2_COMMAND_DISABLE_SCANNING, &response, 1);
}