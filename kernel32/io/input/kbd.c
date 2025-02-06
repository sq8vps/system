#include "kbd.h"
#include "io/log/syslog.h"
#include "io/dev/dev.h"
#include "input.h"
#include "event.h"

STATUS IoKeyboardReportKey(int handle, IoKeyCode code, bool pressed)
{
    union IoEventData d;
    d.keyboard.key = code;
    d.keyboard.state = pressed;
    return IoReportEvent(handle, &d);
}