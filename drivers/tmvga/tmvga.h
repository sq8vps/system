#ifndef DRIVER_TMVGA_H_
#define DRIVER_TMVGA_H_

#include <stdint.h>
#include "common.h" //kernel common

void Tmvga_putChar(char c, Cm_RGBA_t fg, Cm_RGBA_t bg);

void Tmvga_getCursor(uint32_t *col, uint32_t *row);

void Tmvga_setCursor(uint32_t col, uint32_t row);

void Tmvga_resetCursor(void);

void Tmvga_clear(void);

#endif
