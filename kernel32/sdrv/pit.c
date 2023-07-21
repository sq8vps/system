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

    PortIoWriteByte(PIT_CMD_PORT, 0x36);

    PortIoWriteByte(PIT_CH0_PORT, val & 0xFF);
    PortIoWriteByte(PIT_CH0_PORT, val >> 8);
}

void PitOneShotInit(uint32_t time)
{
    uint32_t val = (((uint64_t)time) * ((uint64_t)PIT_CLOCK_FREQUENCY)) / ((uint64_t)1000000);
    PortIoWriteByte(PIT_GATE_PORT, (PortIoReadByte(PIT_GATE_PORT) & 0xFD) | 1);
    PortIoWriteByte(PIT_CMD_PORT, 0xB2); //channel 2, single shot mode
    PortIoWriteByte(PIT_CH2_PORT, val & 0xFF);
    PortIoWriteByte(PIT_CH2_PORT, val >> 8);
}

uint32_t PitOneShotMeasure(uint32_t *currentCounter, uint32_t *initialCounter, uint32_t initial)
{
    uint8_t gate = PortIoReadByte(PIT_GATE_PORT) & 0xFE;
    PortIoWriteByte(PIT_GATE_PORT, gate);
    PortIoWriteByte(PIT_GATE_PORT, gate | 1);
    *initialCounter = initial;
    while(0 == (PortIoReadByte(PIT_GATE_PORT) & 0x20));;
    return initial - (*currentCounter);
}