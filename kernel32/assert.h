#ifndef KERNEL_ASSERT_H_
#define KERNEL_ASSERT_H_

#include "defines.h"

EXPORT_API

#ifdef DEBUG
    #include "common.h"
    /**
     * @brief Check if \a expression evalutes to true. Print error and stop execution if not.
     * @param expression Expression to evaluate
    */
    #define ASSERT(expression) do {if(!(expression)){PRINT("Assertion failed at " __FILE__ ":" STRINGIFY(__LINE__) "\n"); while(1);}} while(0);
#else
    #define ASSERT(expression)
#endif

END_EXPORT_API

#endif