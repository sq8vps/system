#ifndef KERNEL_EVENT_H_
#define KERNEL_EVENT_H_

#include "defines.h"
#include <stdint.h>
#include "kbd.h"

EXPORT_API

struct IoEventHandler;

/**
 * @brief Event/source device type
 */
enum IoEventType
{
    IO_EVENT_UNKNOWN = 0, /**< Unknown event */
    IO_EVENT_KEYBOARD = 1, /**< Keyboard event */

    IO_EVENT_TYPE_COUNT, /**< Count of event types, do not use */
};

/**
 * @brief Event data passed to event notification recipient
 */
union IoEventData
{
    struct IoKeyboardEventData keyboard; /**< Keyboard */
};

/**
 * @brief I/O event notification callback type
 * @param *handler Event handler
 * @param *data Event data
 * @warning It is illegal to modify the handler or data content
 * @note The function is executed at HAL_PRIORITY_LEVEL_DPC priority level
 */
typedef void (*IoEventCallback)(const struct IoEventHandler *handler, const union IoEventData *data);

/**
 * @brief I/O event handler structure
 */
struct IoEventHandler
{
    enum IoEventType type; /**< Event/source device type */
    int handle; /**< Input device handle */
    bool aggregate; /**< Aggregate all events of given type instead of using handle */
    IoEventCallback event; /**< Event notification callback */
    void *context; /**< Additional data */

    struct IoEventHandler *next; /**< Linked list of event handlers */
};

END_EXPORT_API

#endif