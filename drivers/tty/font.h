#ifndef TTY_FONT_H_
#define TTY_FONT_H_

#include "defines.h"

struct TrieNode;
struct TtyVtData;

/**
 * @brief VT font structure
 */
struct TtyFont
{
    uint32_t height; /**< Glyph height in pixels */
    uint32_t width; /**< Glyph width in pixels */
    uint32_t bytesPerRow; /**< Bytes per glyph row */
    uint32_t pitch; /**< Bytes per glyph */
    size_t count; /**< Number of glyphs */
    struct TrieNode *unicodeRoot; /**< Root of the Unicode mapping tree */
    void *unicodeCache; /**< Slab allocator for Unicode mapping tree nodes */

    uint8_t data[]; /**< Glyph data (bitmap) */
};

/**
 * @brief Loads a font from the specified path
 * @param *info Pointer to the TTY VT data structure
 * @param path Path to the font file
 * @return Status code
 */
STATUS TtyLoadFont(struct TtyVtData *info, const char *path);

/**
 * @brief Get a glyph pointer associated with the specified Unicode code points
 * @param *font Pointer to the TTY font structure
 * @param *code Pointer to the array of Unicode code points
 * @param codeCount Number of code points in the array
 * @return Pointer to the glyph data, or NULL if not found
 */
HOT void* TtyGetGlyph(const struct TtyFont *font, const uint32_t *code, size_t codeCount);

/**
 * @brief Draw glyph associated with the specified Unicode code points to the given VT
 * @param *vt Pointer to the TTY VT data structure
 * @param *code Pointer to the array of Unicode code points
 * @param codeCount Number of code points in the array
 * @return Status code
 */
HOT STATUS TtyDrawGlyph(struct TtyVtData *vt, const uint32_t *code, size_t codeCount);

#endif