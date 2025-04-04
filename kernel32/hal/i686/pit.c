#if defined(__i686__) || defined(__amd64__)

#include "pit.h"
#include "ioport.h"
#include "ke/core/mutex.h"

#define PIT_CH0_PORT 0x40
#define PIT_CH2_PORT 0x42
#define PIT_CMD_PORT 0x43
#define PIT_GATE_PORT 0x61

#define PIT_CLOCK_FREQUENCY 1193182 //Hz

static KeSpinlock PitLock = KeSpinlockInitializer;

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

    IoPortWriteByte(PIT_CMD_PORT, 0x36);

    IoPortWriteByte(PIT_CH0_PORT, val & 0xFF);
    IoPortWriteByte(PIT_CH0_PORT, val >> 8);
}

STATUS PitDoSingleShot(uint32_t time, PitCallback start, PitCallback stop, void *context)
{
    //check whether the specified interval can be handled by PIT
    if(time >= (((uint64_t)1000000 * (uint64_t)UINT16_MAX) / (uint64_t)PIT_CLOCK_FREQUENCY))
        return BAD_PARAMETER;
    if(time <= ((uint64_t)1000000 / (uint64_t)PIT_CLOCK_FREQUENCY))
        return BAD_PARAMETER;

    PRIO prio = KeAcquireDpcLevelSpinlock(&PitLock);

    uint16_t cycles = (((uint64_t)time) * ((uint64_t)PIT_CLOCK_FREQUENCY)) / ((uint64_t)1000000);
    IoPortWriteByte(PIT_CMD_PORT, 0xB2); //channel 2, single shot mode
    IoPortWriteByte(PIT_CH2_PORT, cycles & 0xFF);
    IoPortReadByte(PIT_GATE_PORT); //delay
    IoPortWriteByte(PIT_CH2_PORT, cycles >> 8);
    //the initial count is written to the counter register on the next CLK pulse
    //wait for the counter to be updated
    uint16_t v;
    do
    {
        v = IoPortReadByte(PIT_CH2_PORT);
        v |= (((uint16_t)IoPortReadByte(PIT_CH2_PORT)) << 8);
    }
    while(v != cycles);

    //pulse gate low (produce rising edge)
    uint8_t gate = IoPortReadByte(PIT_GATE_PORT) & 0xFE;
    IoPortWriteByte(PIT_GATE_PORT, gate);
    IoPortReadByte(PIT_GATE_PORT); //delay
    IoPortWriteByte(PIT_GATE_PORT, gate | 1);
    //in general, PIT has the OUT pin that is set low when counting begins
    //however, of course, some machines and emulators might not have this pin connected anywhere,
    //thus, there is no sane way to determine whether the couting has started
    //one might check if the current counter value decremented, but this way at least one cycle will be lost

    if(NULL != start)
        start(false, context);

    //the PIT counter counts down and sets OUT high on terminal count (zero)
    //however, since OUT might not be connected, we need to check the counter value directly
    //unfortunately, the timer does not stop on terminal count and begins counting again from 0xFFFF
    //to avoid missing zero, check whether counting is still in progress, that is, if it's between 0 and initial count
    //if not, the counter is 0 or wrapped around and the coutining is finished
    do
    {
        v = IoPortReadByte(PIT_CH2_PORT);
        v |= (((uint16_t)IoPortReadByte(PIT_CH2_PORT)) << 8);
    }
    while((v > 0) && (v <= cycles));

    KeReleaseSpinlock(&PitLock, prio);

    if(NULL != stop)
        stop(true, context);

    return OK;
}

#endif