//This header file is generated automatically
#ifndef EXPORTED___API__ASSERT_H_
#define EXPORTED___API__ASSERT_H_

#ifdef __cplusplus
extern "C" 
{
#endif

#include "defines.h"
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


#ifdef __cplusplus
}
#endif

#endif