#include "vt.h"

void TtyProcessVtInput(const struct IoEventHandler *handler, const union IoEventData *data)
{
    if(unlikely(IO_EVENT_KEYBOARD != handler->type))
        return;
    
    
}