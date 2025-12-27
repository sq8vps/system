/**
 * @file ring.h
 * @brief Ring buffer library
 * @ingroup rtl_ring
 */

#ifndef RTL_RING_H_
#define RTL_RING_H_

#include "defines.h"
#include <stddef.h>

/**
 * @addtogroup rtl_ring Ring buffer library
 * @ingroup rtl
 * @{
 */

EXPORT_API

/**
 * @brief General ring buffer structure
 */
struct RingBuffer
{
    size_t head; /**< Index of the next element to be written */
    size_t tail; /**< Index of the next element to be read */
    size_t capacity; /**< Capacity of the buffer */
    bool full;   /**< Indicates if the buffer is full */
};

/**
 * @brief Initialize the ring buffer
 * @param *ring Pointer to the ring buffer structure
 * @param capacity Capacity of the ring buffer
 */
void RingBufferInitialize(struct RingBuffer *ring, size_t capacity);

/**
 * @brief Get current ring buffer size
 * @param *ring Pointer to the ring buffer
 * @return Current size of the ring buffer
 */
size_t RingBufferGetSize(const struct RingBuffer *ring);

/**
 * @brief Get free space in the ring buffer
 * @param *ring Pointer to the ring buffer
 * @return Free space available in the ring buffer
 */
size_t RingBufferGetFree(const struct RingBuffer *ring);

/**
 * @brief Get new head index for pushing an element into the ring buffer
 * @param *ring Pointer to the ring buffer
 * @return Index where the next element should be pushed
 * @attention This function does not check if the buffer is full
 */
size_t RingBufferGetHeadForPush(struct RingBuffer *ring);

/**
 * @brief Get new tail index for popping an element from the ring buffer
 * @param *ring Pointer to the ring buffer
 * @return Index from where the next element should be popped
 * @attention This function does not check if the buffer is empty
 */
size_t RingBufferGetTailForPop(struct RingBuffer *ring);

/**
 * @brief Get index for peeking the last element in the ring buffer
 * @param *ring Pointer to the ring buffer
 * @return Index of the last element
 * @attention This function does not check if the buffer is empty
 * @attention This function does not alter the ring buffer state
 */
size_t RingBufferGetIndexForPeek(struct RingBuffer *ring);


/**
 * @brief Clear the ring buffer
 * @param *ring Pointer to the ring buffer
 */
void RingBufferClear(struct RingBuffer *ring);

/**
 * @brief Push a value into the ring buffer
 * @param ring Pointer to the ring buffer
 * @param buffer Pointer to the buffer where the value will be stored
 * @param value Value to be pushed into the ring buffer
 */
#define RingBufferPush(ring, buffer, value) buffer[RingBufferGetHeadForPush(ring)] = value

/**
 * @brief Pop a value from the ring buffer
 * @param ring Pointer to the ring buffer
 * @param buffer Pointer to the buffer where the value is stored
 * @return Value popped from the ring buffer
 * @attention This function does not check if the buffer is empty
 */
#define RingBufferPop(ring, buffer) buffer[RingBufferGetTailForPop(ring)]

/**
 * @brief Peek last value pushed to the ring buffer
 * @param ring Pointer to the ring buffer
 * @param buffer Pointer to the buffer where the value is stored
 * @return Last value pushed to the buffer
 * @attention This function does not check if the buffer is empty
 * @attention This function does not alter the ring buffer state
 */
#define RingBufferPeek(ring, buffer) buffer[RingBufferGetIndexForPeek(ring)]

END_EXPORT_API

/**
 * @}
 */

#endif