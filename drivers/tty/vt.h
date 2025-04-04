#ifndef TTY_VT_H_
#define TTY_VT_H_

#include "defines.h"
#include <stdint.h>

#include "io/input/event.h"

/**
 * @brief Process VT input - system event callback
 * @param *handler Event handler
 * @param *data Event data
 */
void TtyProcessVtInput(const struct IoEventHandler *handler, const union IoEventData *data);

#endif