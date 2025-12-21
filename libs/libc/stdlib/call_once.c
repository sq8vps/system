#include "stdlib.h"

#ifdef __GNUC__
void call_once(once_flag *flag, void (*func)(void))
{
    if(0 == __atomic_exchange_n(flag, 1, __ATOMIC_SEQ_CST))
        func();
}
#endif