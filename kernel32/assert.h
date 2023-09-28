#ifndef KERNEL_ASSERT_H_
#define KERNEL_ASSERT_H_

#ifdef DEBUG
    #include "common.h"
    /**
     * @brief Check if \a expression evalutes to true. Print error and stop execution if not.
     * @param expression Expression to evaluate
    */
    #define ASSERT(expression) {if(!(expression)){CmPrintf("Assertion failed at " __FILE__ ":" STRINGIFY(__LINE__) "\n"); while(1);}}
#else
    /**
     * @brief This macro is excluded from build in non-debug mode.
    */
    #define ASSERT(expression)
#endif

#endif