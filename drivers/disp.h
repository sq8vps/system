/*
 * disp.h
 *
 *  Created on: 16.04.2020
 *      Author: Piotr
 */

#ifndef DRIVERS_DISP_H_
#define DRIVERS_DISP_H_

#include <stdint.h>
#include "../kernel/port.h"
#include "../kernel/common.h"

#define DISP_ADDR 0xb8000
#define DISP_PORT_CTRL 0x3D4
#define DISP_PORT_DATA 0x3D5

#define DISP_MAX_ROWS 25
#define DISP_MAX_COLS 80
#define DISP_COLOR_WHITE_ON_BLACK 0x0f


uint8_t disp_printChar(uint8_t c, int16_t col, int16_t row, uint8_t type);

uint16_t disp_getCursor(void);

void disp_setCursor(uint16_t addr);

uint8_t disp_printString(uint8_t *str, int16_t col, int16_t row);

void disp_clear(void);

uint16_t disp_handleScroll(uint16_t cursor);

#endif /* DRIVERS_DISP_H_ */
