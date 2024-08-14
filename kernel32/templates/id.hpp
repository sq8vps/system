#ifndef TEMPLATES_ID_HPP_
#define TEMPLATES_ID_HPP_

extern "C" 
{
    #include "ke/core/mutex.h"
    #include <stddef.h>
}

/**
 * @brief Template class for thread-safe ID dispenser
 * @tparam IdType ID type
 * @tparam MaxIds Number of IDs + 1
 * @attention This class never assigns ID = 0
*/
template <typename IdType, IdType MaxIds>
class IdDispenser
{
private:
    IdType freeStack[MaxIds];
    IdType nextSequential{1};
    IdType freeTop{0};
    KeSpinlock lock KeSpinlockInitializer;
public:
    IdType assign(void)
    {
        IdType id = 0;
        PRIO prio = KeAcquireSpinlock(&lock);
        if(freeTop > 0)
        {
            id = freeStack[freeTop--];
        }
        else if(nextSequential < MaxIds)
        {
            id = nextSequential++;
        }
        KeReleaseSpinlock(&lock, prio);
        return id;
    }
    void free(IdType id)
    {
        PRIO prio = KeAcquireSpinlock(&lock);
        if(freeTop < MaxIds)
        {
            freeStack[++freeTop] = id;
        }
        KeReleaseSpinlock(&lock, prio);
    }
};

#endif