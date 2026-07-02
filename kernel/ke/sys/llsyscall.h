#ifndef KERNEL_SYS_LLSYSCALL_H_
#define KERNEL_SYS_LLSYSCALL_H_

#include "defines.h"

struct KeSyscallDescriptor
{
    void *handler; /**< Target handler function */
    size_t arg[MAX_ARG_COUNT + 1]; /**< Size of each argument. 0 indicates end of argument size list */
};

#define SYSCALL_DESC_NAME(function) __##function##Desc

#define __SYSCALL_DESC(name, ...) const struct KeSyscallDescriptor SYSCALL_DESC_NAME(name) = \
    {.handler = (void*)name, __VA_ARGS__ __VA_OPT__(,) .arg[ARG_COUNT(__VA_ARGS__)] = 0};
#define __ARGSIZE(index, type) .arg[index] = sizeof(type)

#define __DEFINE_SYSCALL0(name) __SYSCALL_DESC(name)
#define __DEFINE_SYSCALL1(name, type0) __SYSCALL_DESC(name, __ARGSIZE(0, type0))
#define __DEFINE_SYSCALL2(name, type0, type1) __SYSCALL_DESC(name, __ARGSIZE(0, type0), __ARGSIZE(1, type1))
#define __DEFINE_SYSCALL3(name, type0, type1, type2) \
    __SYSCALL_DESC(name, __ARGSIZE(0, type0), __ARGSIZE(1, type1), __ARGSIZE(2, type2))
#define __DEFINE_SYSCALL4(name, type0, type1, type2, type3) \
    __SYSCALL_DESC(name, __ARGSIZE(0, type0), __ARGSIZE(1, type1), __ARGSIZE(2, type2), __ARGSIZE(3, type3)) 
#define __DEFINE_SYSCALL5(name, type0, type1, type2, type3, type4) \
    __SYSCALL_DESC(name, __ARGSIZE(0, type0), __ARGSIZE(1, type1), __ARGSIZE(2, type2), __ARGSIZE(3, type3), __ARGSIZE(4, type4))
#define __DEFINE_SYSCALL6(name, type0, type1, type2, type3, type4, type5) \
    __SYSCALL_DESC(name, __ARGSIZE(0, type0), __ARGSIZE(1, type1), __ARGSIZE(2, type2), __ARGSIZE(3, type3), __ARGSIZE(4, type4), __ARGSIZE(5, type5))
#define __DEFINE_SYSCALL7(name, type0, type1, type2, type3, type4, type5, type6) \
    __SYSCALL_DESC(name, __ARGSIZE(0, type0), __ARGSIZE(1, type1), __ARGSIZE(2, type2), __ARGSIZE(3, type3), __ARGSIZE(4, type4), __ARGSIZE(5, type5), \
    __ARGSIZE(6, type6))
#define __DEFINE_SYSCALL8(name, type0, type1, type2, type3, type4, type5, type6, type7) \
    __SYSCALL_DESC(name, __ARGSIZE(0, type0), __ARGSIZE(1, type1), __ARGSIZE(2, type2), __ARGSIZE(3, type3), __ARGSIZE(4, type4), __ARGSIZE(5, type5), \
    __ARGSIZE(6, type6), __ARGSIZE(7, type7))
#define __DEFINE_SYSCALL9(name, type0, type1, type2, type3, type4, type5, type6, type7, typ8) \
    __SYSCALL_DESC(name, __ARGSIZE(0, type0), __ARGSIZE(1, type1), __ARGSIZE(2, type2), __ARGSIZE(3, type3), __ARGSIZE(4, type4), __ARGSIZE(5, type5), \
    __ARGSIZE(6, type6), __ARGSIZE(7, type7), __ARGSIZE(8, type8))
#define __DEFINE_SYSCALL10(name, type0, type1, type2, type3, type4, type5, type6, type7, typ8) \
    __SYSCALL_DESC(name, __ARGSIZE(0, type0), __ARGSIZE(1, type1), __ARGSIZE(2, type2), __ARGSIZE(3, type3), __ARGSIZE(4, type4), __ARGSIZE(5, type5), \
    __ARGSIZE(6, type6), __ARGSIZE(7, type7), __ARGSIZE(8, type8), __ARGSIZE(9, type9))

#define __DEFINE_SYSCALL(returnType, name, argCount, ...) returnType name(__VA_ARGS__); \
    __DEFINE_SYSCALL##argCount(name __VA_OPT__(,) __VA_ARGS__)
    
#define _DEFINE_SYSCALL(returnType, name, argCount, ...) __DEFINE_SYSCALL(returnType, name, argCount, __VA_ARGS__)

/**
 * @brief Define a system call
 * 
 * Declare function \a name and create an associated system call descriptor
 * @param returnType Type returned by the call
 * @param name Function name
 * @param ... Function argument types (unnamed)
 */
#define DEFINE_SYSCALL(returnType, name, ...) _DEFINE_SYSCALL(returnType, name, ARG_COUNT(__VA_ARGS__), __VA_ARGS__);

/**
 * @brief Get syscall descriptor associated with syscall code
 * @param code System call code
 * @return System call descriptor or \a nullptr if code is incorrect
 */
const struct KeSyscallDescriptor* KeGetSyscallDescriptor(size_t code);

#endif