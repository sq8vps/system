extern "C"
{
#include <stdint.h>
#include "ke/core/panic.h"
#include "mm/heap.h"
}

extern char _ctors, _ctors_end;

extern "C" void HalCallConstructors(void)
{
    for(uintptr_t i = 0; i < ((uintptr_t)(&_ctors_end - &_ctors) / sizeof(uintptr_t)); ++i)
	{
		uintptr_t f = ((uintptr_t*)(&_ctors))[i];
		((void(*)())f)();
	}
}

extern "C" void __cxa_pure_virtual()
{
    KePanic(UNEXPECTED_FAULT);
}

void *operator new(size_t size)
{
    return MmAllocateKernelHeap(size);
}

void *operator new[](size_t size)
{
    return MmAllocateKernelHeap(size);
}

void operator delete(void *p)
{
    MmFreeKernelHeap(p);
}

void operator delete[](void *p)
{
    MmFreeKernelHeap(p);
}

// namespace __cxxabiv1 
// {
// #if defined(__i686__)
// 	union __guard
// 	{
// 		KeSpinlock lock;
// 		uint32_t aligner;
// 	};
// #endif
// 	extern "C" int __cxa_guard_acquire (__guard *);
// 	extern "C" void __cxa_guard_release (__guard *);
// 	extern "C" void __cxa_guard_abort (__guard *);

// 	extern "C" int __cxa_guard_acquire (__guard *g) 
// 	{

// 		return 0;
// 	}

// 	extern "C" void __cxa_guard_release (__guard *g)
// 	{
// 		*(char *)g = 1;
// 	}

// 	extern "C" void __cxa_guard_abort (__guard *)
// 	{

// 	}
// }