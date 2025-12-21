#ifndef TTY_PSF_H_
#define TTY_PSF_H_

#include <stdint.h>
#include "defines.h"

struct Psf1Header
{
    uint16_t magic;
    uint8_t mode;
    uint8_t size;
} PACKED;

#define PSF1_MAGIC 0x0436

enum
{
    PSF1_MODE512 = 0x1, /**< 512 glyphs instead of 256 */
    PSF1_MODEHASTAB = 0x2, /**< Unicode table present */
    PSF1_MODESEQ = 0x4 /**< Unicode table present */
};

#define PSF1_SEQ_START 0xFFFE
#define PSF1_ENTRY_END 0xFFFF

struct Psf2Header
{
    uint32_t magic;
    uint32_t version;
    uint32_t headerSize;
    uint32_t flags;
    uint32_t numGlyphs;
    uint32_t glyphSize;
    uint32_t height;
    uint32_t width;
} PACKED;

#define PSF2_MAGIC 0x864AB572

enum
{
    PSF2_HAS_UNICODE_TABLE = 0x1, /**< Unicode table present */
};

#define PSF2_SEQ_START 0xFE
#define PSF2_ENTRY_END 0xFF


#endif