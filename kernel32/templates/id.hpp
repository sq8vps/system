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
template <typename IdType, size_t MaxIds>
class IdDispenser
{
private:
    IdType freeStack[MaxIds];
    IdType nextSequential{1};
    size_t freeTop{0};
    KeSpinlock lock KeSpinlockInitializer;
public:
    IdType assign(void)
    {
        IdType id = 0;
        KeAcquireSpinlock(&lock);
        if(nextSequential < MaxIds)
        {
            id = nextSequential++;
        }
        else if(freeTop > 0)
        {
            id = freeStack[freeTop--];
        }
        KeReleaseSpinlock(&lock);
        return id;
    }
    void free(IdType id)
    {
        KeAcquireSpinlock(&lock);
        if(freeTop < MaxIds)
        {
            freeStack[++freeTop] = id;
        }
        KeReleaseSpinlock(&lock);
    }
};

#endif