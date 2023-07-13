/*
 * disp.h
 *
 *  Created on: 16.04.2020
 *      Author: Piotr
 */

#ifndef DRIVERS_DISP_H_
#define DRIVERS_DISP_H_

#include <stdint.h>
#include "port.h"
#include "common.h"
#include <stdarg.h>

#define DISP_ADDR 0xb8000
#define DISP_PORT_CTRL 0x3D4
#define DISP_PORT_DATA 0x3D5

#define DISP_MAX_ROWS 25
#define DISP_MAX_COLS 80
#define DISP_COLOR_WHITE_ON_BLACK 0x0f
#define DISP_COLOR_DEFAULT (DISP_COLOR_WHITE_ON_BLACK)

#define DISP_NEXT (-1)

#define DISP_GETADDR(c, r) (((((r) * DISP_MAX_COLS) + (c))))


uint8_t disp_printChar(uint8_t c, int16_t col, int16_t row, uint8_t type);

uint16_t disp_getCursor(void);

void disp_setCursor(uint16_t addr);

uint8_t disp_printString(uint8_t *str, int16_t col, int16_t row, uint8_t type);

void disp_clear(void);

uint16_t disp_handleScroll(uint16_t cursor);

uint8_t disp_printHex(uint64_t num, int16_t col, int16_t row, uint8_t type);

uint8_t disp_printHex16(uint16_t num, int16_t col, int16_t row, uint8_t type);

uint8_t disp_printHex8(uint8_t num, int16_t col, int16_t row, uint8_t type);

/**
 * \brief Prints formatted input
 * \param *format Input format
 * \param ... Formatting parameters
 * \warning This function accepts only bare specifiers without flags, width, precision and length parameters.
 */
int printf(const char *format, ...);

#endif /* DRIVERS_DISP_H_ */
