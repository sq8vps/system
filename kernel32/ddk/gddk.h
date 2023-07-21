#ifndef KERNEL_GDDK_H_
#define KERNEL_GDDK_H_

/**
 * @file gddk.h
 * @brief Graphic Driver Development Kit definitions and includes
 * 
 * Provides definitions and includes for graphic driver development
 * 
 * 
*/

#include <stdint.h>
#include "ddk.h"
#include "common.h"


/**
 * @defgroup gddk Graphic Driver Development Kit definitions and includes
 * @ingroup ddk
 * @{
*/

/**
 * @brief Graphic driver callback structure
*/
typedef struct
{
    /**
     * @brief Put char in text mode
     * @param c Character to put
     * @param fg Foreground color
     * @param bg Background color
    */
    void (*textModePutChar)(char c, Cm_RGBA_t fg, Cm_RGBA_t bg);
    /**
     * @brief Clear screen in text mode
    */
    void (*textModeClear)(void);
    /**
     * @brief Set cursor position in text mode
     * @param col Column number
     * @param row Row number
    */
   void (*textModeSetCursorPosition)(uint32_t col, uint32_t row);
   /**
    * @brief Get cursor position in text mode
    * @param col Column number
    * @param row Row number
   */
   void (*textModeGetCursorPosition)(uint32_t *col, uint32_t *row);

    /**
     * @brief Reset cursor to initial position
    */
    void (*textModeResetCursorPosition)(void);
} GDDK_KDrvCallbacks_t;


/**
 * @}
*/

#endif