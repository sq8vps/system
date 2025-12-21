#include "ring.h"

void RingBufferInitialize(struct RingBuffer *ring, size_t capacity)
{
    ring->head = 0;
    ring->tail = 0;
    ring->capacity = capacity;
    ring->full = false;
}

size_t RingBufferGetSize(const struct RingBuffer *ring)
{
    if(ring->full)
        return ring->capacity;

    if(ring->head >= ring->tail)
        return ring->head - ring->tail;
    else
        return ring->capacity - (ring->tail - ring->head);
}

size_t RingBufferGetFree(const struct RingBuffer *ring)
{
    if(ring->full)
        return 0;

    if(ring->head >= ring->tail)
        return ring->capacity - (ring->head - ring->tail);
    else
        return ring->tail - ring->head;
}

size_t RingBufferGetHeadForPush(struct RingBuffer *ring)
{
    size_t old = ring->head;
    ++ring->head;
    ring->head %= ring->capacity;
    if(ring->head == ring->tail)
        ring->full = true;
    return old;
}

size_t RingBufferGetTailForPop(struct RingBuffer *ring)
{
    size_t old = ring->tail;
    ++ring->tail;
    ring->tail %= ring->capacity;
    ring->full = false;
    return old;
}

size_t RingBufferGetIndexForPeek(struct RingBuffer *ring)
{
    if(0 == ring->head)
        return ring->capacity - 1;
    else
        return ring->head - 1;
}

void RingBufferClear(struct RingBuffer *ring)
{
    ring->head = 0;
    ring->tail = 0;
    ring->full = false;
}