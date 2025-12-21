#include "font.h"
#include "psf.h"
#include "rtl/trie.h"
#include "mm/heap.h"
#include "mm/slab.h"
#include "io/fs/fs.h"
#include "rtl/order.h"
#include "rtl/string.h"
#include "vt.h"
#include "unicode/unicode.h"

static STATUS TtyBuildTrieFromPsf1(const void *table, size_t tableSize, struct TtyFont *font)
{
    uint16_t *symbol = (uint16_t*)table; //unicode table entry
    size_t glyph = 0; //glyph index
    size_t i = 0; //table index
    
    do
    {
        //parse non-sequences first
        for(; i < tableSize; i++)
        {
            if((PSF1_ENTRY_END == symbol[i]) || (PSF1_SEQ_START == symbol[i]))
            {
                ++i;
                break;
            }
            uint32_t code = symbol[i];
            struct TrieNode *node = TrieFind(font->unicodeRoot, &code, 1);
            if(NULL == node)
            {
                node = MmSlabAllocate(font->unicodeCache);
                if(NULL == node)
                    return OUT_OF_RESOURCES;

                font->unicodeRoot = TrieInsert(font->unicodeRoot, node, NULL, 0);
            }
            node->key = code;
            node->aux = &font->data[glyph * font->pitch];
        }

        if(PSF1_SEQ_START == symbol[i])
        {
            struct TrieNode *parent = font->unicodeRoot;
            for(; i < tableSize; i++)
            {
                if((PSF1_ENTRY_END == symbol[i]) || (PSF1_SEQ_START == symbol[i]))
                {
                    parent->aux = &font->data[glyph * font->pitch];
                    if(PSF1_ENTRY_END == symbol[i])
                    {
                        ++i;
                        break;
                    }
                }
                uint32_t code = symbol[i];
                struct TrieNode *node = TrieFind(parent, &code, 1);
                if(NULL == node)
                {
                    node = MmSlabAllocate(font->unicodeCache);
                    if(NULL == node)
                        return OUT_OF_RESOURCES;

                    node->key = code;
                    
                    if(NULL == parent)
                        font->unicodeRoot = TrieInsert(font->unicodeRoot, node, NULL, 0);
                    else
                        TrieInsertUnderParent(parent, node);
                }
                parent = node;
            }
        }

        ++glyph;
    } 
    while(i < tableSize);

    return OK;
}

//TODO: PSF2 is completely untested!!!
static STATUS TtyBuildTrieFromPsf2(const void *table, size_t tableSize, struct TtyFont *font)
{
    uint8_t *symbol = (uint8_t*)table; //unicode table entry
    size_t glyph = 0; //glyph index
    size_t i = 0; //table index
    
    do
    {
        //parse non-sequences first
        for(; i < tableSize; i++)
        {
            if((PSF2_ENTRY_END == symbol[i]) || (PSF2_SEQ_START == symbol[i]))
            {
                ++i;
                break;
            }
            uint32_t code = 0;
            size_t byteCount = Utf8ByteCount(symbol[i]);
            if((i + byteCount) > tableSize)
                return CORRUPTED;
            
            switch(byteCount)
            {
                case 1:
                    code = symbol[i];
                    break;
                case 2:
                    code = ((symbol[i] & 0x1F) << 6) | (symbol[i + 1] & 0x3F);
                    i += 1;
                    break;
                case 3:
                    code = ((symbol[i] & 0x0F) << 12) | ((symbol[i + 1] & 0x3F) << 6) | (symbol[i + 2] & 0x3F);
                    i += 2;
                    break;
                case 4:
                    code = ((symbol[i] & 0x07) << 18) | ((symbol[i + 1] & 0x3F) << 12) | ((symbol[i + 2] & 0x3F) << 6) | (symbol[i + 3] & 0x3F);
                    i += 3;
                    break;
                default:
                    return CORRUPTED;
            }

            struct TrieNode *node = TrieFind(font->unicodeRoot, &code, 1);
            if(NULL == node)
            {
                node = MmSlabAllocate(font->unicodeCache);
                if(NULL == node)
                    return OUT_OF_RESOURCES;

                font->unicodeRoot = TrieInsert(font->unicodeRoot, node, NULL, 0);
            }
            node->aux = &font->data[glyph * font->pitch];
        }

        if(PSF2_SEQ_START == symbol[i])
        {
            struct TrieNode *parent = font->unicodeRoot;
            for(; i < tableSize; i++)
            {
                if((PSF2_ENTRY_END == symbol[i]) || (PSF2_SEQ_START == symbol[i]))
                {
                    parent->aux = &font->data[glyph * font->pitch];
                    if(PSF1_ENTRY_END == symbol[i])
                    {
                        ++i;
                        break;
                    }
                }
                uint32_t code = 0;
                size_t byteCount = Utf8ByteCount(symbol[i]);
                if((i + byteCount) > tableSize)
                    return CORRUPTED;
                
                switch(byteCount)
                {
                    case 1:
                        code = symbol[i];
                        break;
                    case 2:
                        code = ((symbol[i] & 0x1F) << 6) | (symbol[i + 1] & 0x3F);
                        i += 1;
                        break;
                    case 3:
                        code = ((symbol[i] & 0x0F) << 12) | ((symbol[i + 1] & 0x3F) << 6) | (symbol[i + 2] & 0x3F);
                        i += 2;
                        break;
                    case 4:
                        code = ((symbol[i] & 0x07) << 18) | ((symbol[i + 1] & 0x3F) << 12) | ((symbol[i + 2] & 0x3F) << 6) | (symbol[i + 3] & 0x3F);
                        i += 3;
                        break;
                    default:
                        return CORRUPTED;
                }
                struct TrieNode *node = TrieFind(parent, &code, 1);
                if(NULL == node)
                {
                    node = MmSlabAllocate(font->unicodeCache);
                    if(NULL == node)
                        return OUT_OF_RESOURCES;

                    if(NULL == parent)
                        font->unicodeRoot = TrieInsert(font->unicodeRoot, node, NULL, 0);
                    else
                        TrieInsertUnderParent(parent, node);
                }
                parent = node;
            }
        }

        ++glyph;
    } 
    while(i < tableSize);

    return OK;
}

STATUS TtyLoadFont(struct TtyVtData *info, const char *path)
{
    int f = -1;
    STATUS status = OK;
    bool hasTable = false;
    bool isPsf2 = false;
    struct TtyFont *font = NULL;
    void *table = NULL;
    uint8_t buf[sizeof(struct Psf2Header)];
    struct Psf1Header *psf1h = (struct Psf1Header*)buf;
    struct Psf2Header *psf2h = (struct Psf2Header*)buf;
    size_t glyphCount = 0;
    size_t headerSize = 0;

    status = IoOpenFile(path, IO_FILE_READ, IO_FILE_FLAG_SHARED, &f);
    if(OK != status)
        return status;
    
    status = IoReadFileSync(f, buf, sizeof(buf), 0, NULL);
    if(OK != status)
        goto TtyLoadFontFail;

    if(PSF1_MAGIC == RtlLeU16(psf1h->magic))
    {
        if(0 == psf1h->size)
        {
            status = CORRUPTED;
            goto TtyLoadFontFail;
        }
        glyphCount = (psf1h->mode & PSF1_MODE512) ? 512 : 256;
        headerSize = sizeof(struct Psf1Header);
    }
    else if(PSF2_MAGIC == RtlLeU32(psf2h->magic))
    {
        isPsf2 = true;
        glyphCount = psf2h->numGlyphs;
        headerSize = psf2h->headerSize;
    }
    else
    {
        status = BAD_TYPE;
        goto TtyLoadFontFail;
    }

    font = MmAllocateKernelHeapZeroed(sizeof(*font) + glyphCount * (isPsf2 ? psf2h->glyphSize : psf1h->size));
    if(NULL == font)
    {
        status = OUT_OF_RESOURCES;
        goto TtyLoadFontFail;
    }

    font->height = isPsf2 ? psf2h->height : psf1h->size;
    font->width = isPsf2 ? psf2h->width : 8;
    font->bytesPerRow = isPsf2 ? CEIL_DIV(psf2h->width, 8) : 1;
    font->pitch = isPsf2 ? psf2h->glyphSize : psf1h->size;
    font->count = glyphCount;

    font->unicodeCache = MmSlabCreate(sizeof(*font->unicodeRoot), 128);
    if(NULL == font->unicodeCache)
    {
        status = OUT_OF_RESOURCES;
        goto TtyLoadFontFail;
    }

    status = IoReadFileSync(f, font->data, font->pitch * glyphCount, headerSize, NULL);
    if(OK != status)
        goto TtyLoadFontFail;

    hasTable = !!(isPsf2 ? (psf2h->flags & PSF2_HAS_UNICODE_TABLE) : ((psf1h->mode & PSF1_MODEHASTAB) || (psf1h->mode & PSF1_MODESEQ)));

    if(hasTable)
    {
        uint64_t tableSize = 0;
        status = IoGetFileSize(path, &tableSize);
        if(OK != status)
            goto TtyLoadFontFail;
        else if(tableSize < (headerSize + (font->pitch * font->count)))
        {
            status = CORRUPTED;
            goto TtyLoadFontFail;
        }

        tableSize -= (headerSize + (font->pitch * font->count));

        table = MmAllocateKernelHeap(tableSize);
        if(NULL == table)
        {
            status = OUT_OF_RESOURCES;
            goto TtyLoadFontFail;
        }

        status = IoReadFileSync(f, table, tableSize, headerSize + (font->pitch * font->count), NULL);
        if(OK != status)
            goto TtyLoadFontFail;

        if(!isPsf2)
            tableSize /= sizeof(uint16_t); //reuse tableSize so that it represents number of entries in the table

        status = isPsf2 ? TtyBuildTrieFromPsf2(table, tableSize, font) : TtyBuildTrieFromPsf1(table, tableSize, font);
    }
    else //no unicode table
    {
        for(size_t i = 0; i < glyphCount; ++i)
        {
            struct TrieNode *node = MmSlabAllocate(font->unicodeCache);
            if(NULL == node)
            {
                status = OUT_OF_RESOURCES;
                goto TtyLoadFontFail;
            }
            node->aux = &font->data[i * font->pitch];
            font->unicodeRoot = TrieInsert(font->unicodeRoot, node, NULL, 0);
        }
        if(NULL == font->unicodeRoot)
        {
            status = OUT_OF_RESOURCES;
            goto TtyLoadFontFail;
        }
    }

    info->output.font = font;
    info->output.limits.x = info->output.config.fb.config.width / font->width;
    info->output.limits.y = info->output.config.fb.config.height / font->height;

    IoCloseFile(f);
    MmFreeKernelHeap(table);
    return OK;

TtyLoadFontFail:
    IoCloseFile(f);
    MmFreeKernelHeap(table);
    if(NULL != font)
    {
        if(NULL != font->unicodeCache)
            MmSlabDestroy(font->unicodeCache);
        MmFreeKernelHeap(font);
    }
    return status;
}

HOT inline void* TtyGetGlyph(const struct TtyFont *font, const uint32_t *code, size_t codeCount)
{
    struct TrieNode *node = TrieFind(font->unicodeRoot, code, codeCount);
    if(NULL == node)
        return NULL;

    return node->aux;
}


HOT STATUS TtyDrawGlyph(struct TtyVtData *vt, const uint32_t *code, size_t codeCount)
{
    const struct TtyFont *font = vt->output.font;

    uint8_t *glyph = TtyGetGlyph(font, code, codeCount);
    if(NULL == glyph)
        return OK;
    
    if(likely(IO_VIDEO_FRAME_BUFFER == vt->output.type)) //maybe remove "likely" when we have more output types
    {
        struct IoFrameBufferConfig *fb = &vt->output.config.fb.config;

        //we don't support non-integer number of bytes per pixel, so assume it is divisible by 8
        //x and y represent top leftmost pixel address of the glyph
        size_t x = (size_t)vt->output.cursor.x * (size_t)font->width * (size_t)vt->output.bytesPerPixel;
        size_t y = (size_t)vt->output.cursor.y * (size_t)font->height * (size_t)fb->pitch;

        for(uint32_t i = 0; i < font->height; i++)
        {
            size_t yShift = y + i * fb->pitch; //address shift coming from the Y position
            uint8_t byte = 0;
            for(uint32_t k = 0; k < font->width; k++)
            {
                if(0 == (k % 8))
                    byte = glyph[(i * font->bytesPerRow) + (k / 8)]; //get the next byte on byte boundary crossing
                
                for(size_t m = 0; m < vt->output.bytesPerPixel; m++)
                {
                    uint8_t *pixel = &((uint8_t*)vt->output.config.fb.fb)[yShift + x + k * vt->output.bytesPerPixel + m];
                    
                    *pixel &= ~(vt->output.mask >> (m * 8));
                    *pixel |= ((byte & 0x80) ? vt->output.fgPixel : vt->output.bgPixel) >> (m * 8);
                }

                byte <<= 1;
            }
        }
    }
    return OK;
}