#include "kbd.h"
#include "scancodes.h"
#include "ps2.h"
#include "io/dev/rp.h"
#include "io/dev/dev.h"
#include "io/input/kbd.h"

#define PS2_CMD_KBD_SET_LEDS 0xED
#define PS2_CMD_KBD_SET_SCAN_CODE_SET 0xF0
#define PS2_CMD_KBD_SET_TYPEMATIC_CONFIG 0xF3

uint16_t Ps2KeyboardParse(uint8_t data, uint8_t *buffer, uint8_t *index)
{
    buffer[*index] = data;
    (*index)++;
    if(1 == *index) //first byte
    {
        if((0xF0 == data) || (0xE0 == data) || (0xE1 == data)) //break or extended code, wait for the next byte
        {
            return PS2_KEY_INCOMPLETE;
        }
        else //not an extended code, the scan code is 1 byte wide
        {
            *index = 0;
            if(data >= (sizeof(Ps2UsSet2) / sizeof(*Ps2UsSet2)))
                return PS2_KEY_STATE_BIT;
            else
                return Ps2UsSet2[data] | PS2_KEY_STATE_BIT;
        }
    }
    else if(2 == *index) //second byte
    {
        if(0xF0 == buffer[0]) //first byte is 0xF0, which is break (key released)
        {
            //key is not extended, otherwise 0xE0 would be first
            *index = 0;
            if(data >= (sizeof(Ps2UsSet2) / sizeof(*Ps2UsSet2)))
                return 0;
            else
                return Ps2UsSet2[data];
        }
        else if(0xE0 == buffer[0]) //first byte is 0xE0, so the key is extended
        {
            if((0xF0 == data) || (0x12 == data)) 
            {
                //if current byte is 0xF0, which is break (key released), then wait for the actual key code
                //if current byte is 0x12, this should be print screen key, wait
                return PS2_KEY_INCOMPLETE;
            }
            else //key is extended
            {
                *index = 0;
                if(data >= (sizeof(Ps2UsSet2Ext) / sizeof(*Ps2UsSet2Ext)))
                    return 0;
                else
                    return Ps2UsSet2Ext[data] | PS2_KEY_STATE_BIT;
            }
        }
        else //if(0xE1 == buffer[0]), only pause key starts with 0xE1
        {
            return PS2_KEY_INCOMPLETE; //wait for all bytes
        }
    }
    else if(3 == *index) //third byte
    {
        //normal combination for extended key break?
        if((0xE0 == buffer[0]) && (0xF0 == buffer[1]))
        {
            if(0x7C == data) //print screen break, wait
                return PS2_KEY_INCOMPLETE;

            *index = 0;
            if(data >= (sizeof(Ps2UsSet2Ext) / sizeof(*Ps2UsSet2Ext)))
                return 0;
            else
                return Ps2UsSet2Ext[data];
        }
        //print screen combination, wait
        else if((0xE0 == buffer[0]) && (0x12 == buffer[1]) && (0xE0 == data))
        {
            return PS2_KEY_INCOMPLETE;
        }
        //pause combination, wait
        else if((0xE1 == buffer[0]) && (0x14 == buffer[1]) && (0x77 == data))
        {
            return PS2_KEY_INCOMPLETE;
        }
        else
        {
            *index = 0;
            return 0; //key unknown
        }
    }
    else //4th and next bytes
    {
        //print screen make
        if((0xE0 == buffer[0]) && (0x12 == buffer[1]) && (0xE0 == buffer[2]) && (0x7C == data))
        {
            *index = 0;
            return IO_KEY_PRINT_SCREEN | PS2_KEY_STATE_BIT;
        }
        //print screen break
        else if((6 == *index) 
            && (0xE0 == buffer[0]) && (0xF0 == buffer[1]) && (0x7C == buffer[2]) 
            && (0xE0 == buffer[3]) && (0xF0 == buffer[4]) && (0x12 == buffer[5]))
        {
            *index = 0;
            return IO_KEY_PRINT_SCREEN;
        }
        //pause
        else if((8 == *index) 
            && (0xE1 == buffer[0]) && (0x14 == buffer[1]) && (0x77 == buffer[2]) 
            && (0xE1 == buffer[3]) && (0xF0 == buffer[4]) && (0x14 == buffer[5])
            && (0xF0 == buffer[6]) && (0x77 == buffer[7]))
        {
            *index = 0;
            return IO_KEY_PAUSE | PS2_KEY_PULSE_BIT;
        }
        //unknown
        else if(8 == *index)
        {
            *index = 0;
            return 0;
        }
        //wait until 8 bytes are received
        else
        {
            return PS2_KEY_INCOMPLETE;
        }
    }
}

void Ps2KeyboardSetLeds(enum Ps2KeyboardLed state, struct IoRp *rp)
{
    uint8_t data[2] = {PS2_CMD_KBD_SET_LEDS, (uint8_t)state};
    Ps2SendCommand((struct I8042Peripheral*)(rp->device->privateData), data, 2, rp);
}

bool Ps2KeyboardSetUp(struct I8042Peripheral *info)
{
    uint8_t data[2];
    data[0] = PS2_CMD_KBD_SET_SCAN_CODE_SET;
    data[1] = 2; //set scan code set 2
    if(Ps2WriteMultiple(info, data, 2))
    {
        //the kernel should do repetition in software
        //hardware repetition might be broken
        //there is no way to disable it completely, but set the delay to max and rate to min
        data[0] = PS2_CMD_KBD_SET_TYPEMATIC_CONFIG;
        data[1] = 0x7F; //[7] = 0b0, [6:5] = 0b11 (delay = 1s), [4:0] = 0b1111 (rate = 2Hz)
        return Ps2WriteMultiple(info, data, 2);
    }
    return false;
}