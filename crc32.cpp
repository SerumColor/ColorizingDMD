#include "crc32.h"

uint32_t crc32_table[256];

void build_crc32_table(void) 
{
    for (uint32_t i = 0; i < 256; i++)
    {
        uint32_t ch = i;
        uint32_t crc = 0;
        for (size_t j = 0; j < 8; j++)
        {
            uint32_t b = (ch ^ crc) & 1;
            crc >>= 1;
            if (b) crc = crc ^ 0xEDB88320;
            ch >>= 1;
        }
        crc32_table[i] = crc;
    }
}

uint32_t crc32_fast(const UINT8* s, size_t n, BOOL ShapeMode) 
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < n; i++)
    {
        UINT8 val = s[i];
        if ((ShapeMode == TRUE) && (val > 1)) val = 1;
        crc = (crc >> 8) ^ crc32_table[(val ^ crc) & 0xFF];
    }
    return ~crc;
}

uint32_t crc32_fast_step(const UINT8* s, const UINT step, size_t n, BOOL ShapeMode) 
{
    uint32_t crc = 0xFFFFFFFF;
    UINT j = 0;
    for (size_t i = 0; i < n; i++)
    {
        UINT8 val = s[j];
        j += step;
        if ((ShapeMode == TRUE) && (val > 1)) val = 1;
        crc = (crc >> 8) ^ crc32_table[(val ^ crc) & 0xFF];
    }
    return ~crc;
}

uint32_t crc32_fast_count(const UINT8* s, size_t n, BOOL ShapeMode, UINT8* pncols) 

{
    *pncols = 0;
    bool usedcolors[256];
    memset(usedcolors, false, 256);
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < n; i++)
    {
        UINT8 val = s[i];
        if (!usedcolors[val])
        {
            usedcolors[val] = true;
            (*pncols)++;
        }
        if ((ShapeMode == TRUE) && (val > 1)) val = 1;
        crc = (crc >> 8) ^ crc32_table[(val ^ crc) & 0xFF];
    }
    return ~crc;
}

uint32_t crc32_fast_mask_shape(const UINT8* source, const UINT8* mask, size_t n, BOOL ShapeMode) 

{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < n; i++)
    {
        if (mask[i] == 0)
        {
            UINT8 val = source[i];
            if ((ShapeMode == TRUE) && (val > 1)) val = 1;
            crc = (crc >> 8) ^ crc32_table[(val ^ crc) & 0xFF];
        }
    }
    return ~crc;
}

uint32_t crc32_fast_mask(const UINT8* source, const UINT8* mask, size_t n) 
{
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < n; i++)
    {
        if (mask[i] != 0)
        {
            crc = (crc >> 8) ^ crc32_table[(source[i] ^ crc) & 0xFF];
        }
    }
    return ~crc;
}