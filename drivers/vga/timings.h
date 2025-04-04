#ifndef VGA_TIMINGS_H_
#define VGA_TIMINGS_H_

#include "vga.h"

#define VGA_MODE_FLAGS(name, clk, xTotal, yTotal, xUsable, yUsable, xPorch, yPorch, xSync, ySync, interlaced_, hSync_, vSync_) \
    [name] = {.clock = clk, .total = {.x = xTotal, .y = yTotal}, .usable = {.x = xUsable, .y = yUsable}, .porch = {.x = xPorch, .y = yPorch}, \
    .sync = {.x = xSync, .y = ySync}, .flags = {.interlaced = interlaced_, .hSync = hSync_, .vSync = vSync_}}

#define VGA_MODE(name, clk, xTotal, yTotal, xUsable, yUsable, xPorch, yPorch, xSync, ySync) \
    VGA_MODE_FLAGS(name, clk, xTotal, yTotal, xUsable, yUsable, xPorch, yPorch, xSync, ySync, 0, 0, 0)

enum
{
    VESA_800_600_60 = 0,
    VESA_800_600_56 = 1,
    VESA_640_480_75 = 2,
    VESA_640_480_72 = 3,
    VGA_640_480_60 = 4,

    VESA_1280_1024_75 = 5,
    VESA_1024_768_75 = 6,
    VESA_1024_768_70 = 7,
    VESA_1024_768_60 = 8,
    VESA_800_600_75 = 9,
    VESA_800_600_72 = 10,
};

static const struct VgaTiming VgaTimings[] = 
{
    VGA_MODE(VESA_800_600_60, 40000000, 1056, 628, 800, 600, 40, 1, 128, 4),
    VGA_MODE(VESA_800_600_56, 36000000, 1024, 625, 800, 600, 24, 1, 72, 2),
    VGA_MODE_FLAGS(VESA_640_480_75, 31500000, 840, 500, 640, 480, 16, 1, 64, 3, 0, 1, 1),
    VGA_MODE_FLAGS(VESA_640_480_72, 31500000, 832, 520, 640, 480, 16, 1, 40, 3, 0, 1, 1),
    VGA_MODE_FLAGS(VGA_640_480_60, 25175000, 800, 525, 640, 480, 16, 10, 96, 2, 0, 1, 1),

    VGA_MODE(VESA_1280_1024_75, 135000000, 1688, 1066, 1280, 1024, 16, 1, 144, 3),
    VGA_MODE(VESA_1024_768_75, 78750000, 1312, 800, 1024, 768, 16, 1, 96, 3),
    VGA_MODE_FLAGS(VESA_1024_768_70, 75000000, 1328, 806, 1024, 768, 24, 3, 136, 6, 0, 1, 1),
    VGA_MODE_FLAGS(VESA_1024_768_60, 65000000, 1344, 806, 1024, 768, 24, 3, 136, 6, 0, 1, 1),
    VGA_MODE(VESA_800_600_75, 49500000, 1056, 625, 800, 600, 16, 1, 80, 3),
    VGA_MODE(VESA_800_600_72, 50000000, 1040, 666, 800, 600, 56, 37, 120, 6),
};

#endif