#ifndef VGA_VESA_H_
#define VGA_VESA_H_

#include "defines.h"

struct VgaAdapterInfo;
struct VgaDisplayInfo;
struct VgaTiming;

/**
 * @brief Get VBE-compatible adapter info
 * @param *info VGA adapter structure to be filled
 * @return Status code
 */
STATUS VgaVesaGetAdapterInfo(struct VgaAdapterInfo *info);

/**
 * @brief Fill display info structure
 * @param *info Display structure to be filled
 * @return Status code
 */
STATUS VgaVesaGetDisplayInfo(struct VgaDisplayInfo *info);

/**
 * @brief Set video mode
 * @param mode Target VBE mode number
 * @param *timing Timing parameters (optional, might be NULL)
 * @return Status code
 * @warning This function does not check for adapter-display mode compatibility
 */
STATUS VgaVesaSetMode(uint16_t vbeMode, const struct VgaTiming *timing);



#endif