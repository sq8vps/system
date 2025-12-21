#include "time.h"
#include "mm/heap.h"
#include "ke/core/mutex.h"

struct HalClock
{
    struct HalClockSource *params; /**< Clock source parameters */
    uint64_t lastTicks; /**< Last count of ticks */
    uint64_t lastTime; /**< Last timestamp in nanoseconds */
    uint32_t mult; /**< Multiplier for calculating nanoseconds from ticks */
    uint8_t shift; /**< Shift for calculating nanoseconds from ticks */
    KeSeqCounter seqCounter; /**< Sequence counter for protecting last ticks and time values */

    struct HalClock *next; /**< Next clock source */
};

static struct
{
    struct HalClock *clock; /**< List of clock sources */
    struct HalClock *best; /**< Current best clock */


    KeSpinlock lock; /**< Structure lock */
} HalClockState;


STATUS HalRegisterClockSource(struct HalClockSource *cs)
{
    if((NULL == cs->read) || (0 == cs->frequency))
        return BAD_PARAMETER;

    struct HalClock *c = MmAllocateKernelHeapZeroed(sizeof(*c));
    if(NULL == c)
        return OUT_OF_RESOURCES;

    //find the biggest shift for which the multiplier fits in 32 bits
    //shift must be at most 30, as 10e9 shifted by 31 does not fit in 64 bits
    uint8_t shift = 30;
    uint64_t mult;
    while(1)
    {
        mult = (uint64_t)1000000000 << shift;
        mult /= cs->frequency;
        if(mult <= UINT32_MAX)
            break;
        --shift;
    }
    
    c->mult = (uint32_t)mult;
    c->shift = shift;
    c->params = cs;
    cs->control = c;

    PRIO prio = KeAcquireDpcLevelSpinlock(&HalClockState.lock);
    if(NULL == HalClockState.clock)
        HalClockState.clock = c;
    else
    {
        struct HalClock *t = HalClockState.clock;
        while(NULL != t->next)
            t = t->next;

        t->next = c;
    }
    KeReleaseSpinlock(&HalClockState.lock, prio);

    HalUpdateClockSource(cs);

    return OK;
}

static inline uint64_t HalCalculateNewTimestamp(const struct HalClock *c, uint64_t currentTicks)
{
    //assume ticks delta is small enough to fit within 32 bits
    //at 10 GHz the delta is at most 429.5 ms
    uint32_t delta = (uint32_t)(currentTicks - c->lastTicks);
    return (((uint64_t)c->mult * (uint64_t)delta) >> c->shift) + c->lastTime;   
}

static inline void HalSelectBestClock(const struct HalClockSource *cs)
{
    if(likely(NULL != HalClockState.best))
    {
        if(unlikely(HalClockState.best->params->rating < cs->rating))
            HalClockState.best = cs->control;
    }
    else
        HalClockState.best = cs->control;
}

void HalUpdateClockSource(struct HalClockSource *cs)
{
    if(unlikely(NULL == cs->control))
        return;
    
    struct HalClock *c = cs->control;

    KeSeqCounterWriteBegin(&c->seqCounter);
    uint64_t currentTicks = cs->read(cs->context);
    c->lastTime = HalCalculateNewTimestamp(c, currentTicks);
    c->lastTicks = currentTicks;
    KeSeqCounterWriteEnd(&c->seqCounter);

    HalSelectBestClock(cs);
}

uint64_t HalGetTimestampEx(struct HalClockSource *cs)
{
    uint64_t timestamp;
    struct HalClock *c = cs->control;

    KeSeqCount seq = KeSeqCounterReadBegin(&c->seqCounter);
    do
    {
        timestamp = HalCalculateNewTimestamp(c, cs->read(cs->context));
    }
    while(KeSeqCounterReadRetry(&c->seqCounter, &seq));

    return timestamp;
}

uint64_t HalGetTimestamp(void)
{
    return HalGetTimestampEx(HalClockState.best->params);
}

uint64_t HalGetTimestampMicros(void)
{
    return HalGetTimestampEx(HalClockState.best->params) / (uint64_t)1000;
}

uint64_t HalGetTimestampMillis(void)
{
    return HalGetTimestampEx(HalClockState.best->params) / (uint64_t)1000000;
}

