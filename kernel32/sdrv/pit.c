#include "pit.h"
#include "hal/ioport.h"

#define PIT_CH0_PORT 0x40
#define PIT_CH2_PORT 0x42
#define PIT_CMD_PORT 0x43
#define PIT_GATE_PORT 0x61

#define PIT_CLOCK_FREQUENCY 1193182 //Hz

void PitInit(void)
{
    
}

void PitSetInterval(uint32_t interval)
{
    uint32_t val = ((uint64_t)interval) * ((uint64_t)PIT_CLOCK_FREQUENCY) / ((uint64_t)1000);
    if(val < 2)
        val = 2;
    if(val > 65535)
        val = 65535;

    HalIoPortWriteByte(PIT_CMD_PORT, 0x36);

    HalIoPortWriteByte(PIT_CH0_PORT, val & 0xFF);
    HalIoPortWriteByte(PIT_CH0_PORT, val >> 8);
}

//one-shot mode terminal counter value
static uint16_t oneShotValue = 0;

void PitOneShotInit(uint32_t time)
{
    oneShotValue = (((uint64_t)time) * ((uint64_t)PIT_CLOCK_FREQUENCY)) / ((uint64_t)1000000);
    HalIoPortWriteByte(PIT_CMD_PORT, 0xB2); //channel 2, single shot mode
    HalIoPortWriteByte(PIT_CH2_PORT, oneShotValue & 0xFF);
    HalIoPortReadByte(PIT_GATE_PORT); //delay
    HalIoPortWriteByte(PIT_CH2_PORT, oneShotValue >> 8);
}

void PitOneShotStart(void)
{
    //pulse gate low (produce rising edge)
    uint8_t gate = HalIoPortReadByte(PIT_GATE_PORT) & 0xFE;
    HalIoPortWriteByte(PIT_GATE_PORT, gate);
    HalIoPortReadByte(PIT_GATE_PORT); //delay
    HalIoPortWriteByte(PIT_GATE_PORT, gate | 1);
    //wait for the counter to start counting
    //that is wait until the counter is different than initial value
    //it would be best to use timer output at port 0x61, though e.g. Qemu does not have this output connected
    uint16_t v;
    do
    {
        v = HalIoPortReadByte(PIT_CH2_PORT);
        v |= (((uint16_t)HalIoPortReadByte(PIT_CH2_PORT)) << 8);
    }
    while(v == oneShotValue);
}

void PitOneShotWait(void)
{
    //wait for the counter to reach terminal value
    uint16_t v = 0;
    while(v < oneShotValue)
    {
        v = HalIoPortReadByte(PIT_CH2_PORT);
        v |= (((uint16_t)HalIoPortReadByte(PIT_CH2_PORT)) << 8);
    }
}