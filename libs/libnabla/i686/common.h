#ifndef NABLA_I686_COMMON_H_
#define NABLA_I686_COMMON_H_

#include <stdint.h>
#include "defines.h"

typedef uint32_t reg_t;

#define __ARGSIZE(type) ALIGN_UP(sizeof(type), sizeof(reg_t))

#define __SYSCALL(returnType, name, argSize, ...) {return (returnType)__ApiDoSyscall(argSize, GET_SYSCALL_NUMBER(name)__VA_OPT__(,) __VA_ARGS__);}

#define __SYSCALL_WRAPPER0(returnType, name) \
    returnType name(void) __SYSCALL(returnType, name, 0)
#define __SYSCALL_WRAPPER1(returnType, name, type0) \
    returnType name(type0 arg0) \
        __SYSCALL(returnType, name, __ARGSIZE(type0),\
        arg0)
#define __SYSCALL_WRAPPER2(returnType, name, type0, type1) \
    returnType name(type0 arg0, type1 arg1) \
        __SYSCALL(returnType, name, __ARGSIZE(type0) + __ARGSIZE(type1),\
        arg0, arg1)
#define __SYSCALL_WRAPPER3(returnType, name, type0, type1, type2) \
    returnType name(type0 arg0, type1 arg1, type2 arg2) \
        __SYSCALL(returnType, name, __ARGSIZE(type0) + __ARGSIZE(type1) + __ARGSIZE(type2),\
        arg0, arg1, arg2)
#define __SYSCALL_WRAPPER4(returnType, name, type0, type1, type2, type3) \
    returnType name(type0 arg0, type1 arg1, type2 arg2, type3 arg3) \
        __SYSCALL(returnType, name, __ARGSIZE(type0) + __ARGSIZE(type1) + __ARGSIZE(type2) + __ARGSIZE(type3),\
        arg0, arg1, arg2, arg3)
#define __SYSCALL_WRAPPER5(returnType, name, type0, type1, type2, type3, type4) \
    returnType name(type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4) \
        __SYSCALL(returnType, name, __ARGSIZE(type0) + __ARGSIZE(type1) + __ARGSIZE(type2) + __ARGSIZE(type3) + __ARGSIZE(type4),\
        arg0, arg1, arg2, arg3, arg4)
#define __SYSCALL_WRAPPER6(returnType, name, type0, type1, type2, type3, type4, type5) \
    returnType name(type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5) \
        __SYSCALL(returnType, name, __ARGSIZE(type0) + __ARGSIZE(type1) + __ARGSIZE(type2) + __ARGSIZE(type3) + __ARGSIZE(type4) + __ARGSIZE(type5),\
        arg0, arg1, arg2, arg3, arg4, arg5)
#define __SYSCALL_WRAPPER7(returnType, name, type0, type1, type2, type3, type4, type5, type6) \
    returnType name(type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6) \
        __SYSCALL(returnType, name, __ARGSIZE(type0) + __ARGSIZE(type1) + __ARGSIZE(type2) + __ARGSIZE(type3) + __ARGSIZE(type4) + __ARGSIZE(type5) \
            + __ARGSIZE(type6), arg0, arg1, arg2, arg3, arg4, arg5, arg6)
#define __SYSCALL_WRAPPER8(returnType, name, type0, type1, type2, type3, type4, type5, type6, type7) \
    returnType name(type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7) \
        __SYSCALL(returnType, name, __ARGSIZE(type0) + __ARGSIZE(type1) + __ARGSIZE(type2) + __ARGSIZE(type3) + __ARGSIZE(type4) + __ARGSIZE(type5) \
            + __ARGSIZE(type6) + __ARGSIZE(type7), arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7)
#define __SYSCALL_WRAPPER9(returnType, name, type0, type1, type2, type3, type4, type5, type6, type7, type8) \
    returnType name(type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8) \
        __SYSCALL(returnType, name, __ARGSIZE(type0) + __ARGSIZE(type1) + __ARGSIZE(type2) + __ARGSIZE(type3) + __ARGSIZE(type4) + __ARGSIZE(type5) \
            + __ARGSIZE(type6) + __ARGSIZE(type7) + __ARGSIZE(type8), arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8)
#define __SYSCALL_WRAPPER10(returnType, name, type0, type1, type2, type3, type4, type5, type6, type7, type8, type9) \
    returnType name(type0 arg0, type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6, type7 arg7, type8 arg8, type9 arg9) \
        __SYSCALL(returnType, name, __ARGSIZE(type0) + __ARGSIZE(type1) + __ARGSIZE(type2) + __ARGSIZE(type3) + __ARGSIZE(type4) + __ARGSIZE(type5) \
            + __ARGSIZE(type6) + __ARGSIZE(type7) + __ARGSIZE(type8) + __ARGSIZE(type9), arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9)

#define __SYSCALL_WRAPPER(returnType, name, argCount, ...) __SYSCALL_WRAPPER##argCount(returnType, name __VA_OPT__(,) __VA_ARGS__)

#define _SYSCALL_WRAPPER(returnType, name, argCount, ...) __SYSCALL_WRAPPER(returnType, name, argCount, __VA_ARGS__)

#define SYSCALL_WRAPPER(returnType, name, ...) _SYSCALL_WRAPPER(returnType, name, ARG_COUNT(__VA_ARGS__), __VA_ARGS__);
#endif