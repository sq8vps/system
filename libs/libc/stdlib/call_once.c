#include "stdlib.h"

#ifdef __GNUC__
void call_once(once_flag *flag, void (*func)(void))
{
    if(0 == ATOMIC_EXCHANGE(flag, 1, ATOMIC_SEQ_CST))
        func();
}
#endif