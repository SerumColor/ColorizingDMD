#ifndef CRC32_H
#define CRC32_H

#include "targetver.h"
#include <stdint.h>
#include <basetsd.h>
#include <windows.h>

void build_crc32_table(void); // initiating the CRC table, must be called at startup

uint32_t crc32_fast(const UINT8* s, size_t n, BOOL ShapeMode); // computing a buffer CRC32, "build_crc32_table()" must have been called before the first use

uint32_t crc32_fast_step(const UINT8* s, const UINT step, size_t n, BOOL ShapeMode); // computing a buffer CRC32, "build_crc32_table()" must have been called before the first use

uint32_t crc32_fast_count(const UINT8* s, size_t n, BOOL ShapeMode, UINT8* pncols); // computing a buffer CRC32, "build_crc32_table()" must have been called before the first use
// this version counts the number of different values found in the buffer

uint32_t crc32_fast_mask_shape(const UINT8* source, const UINT8* mask, size_t n, BOOL ShapeMode); // computing a buffer CRC32 on the non-masked area, "build_crc32_table()" must have been called before the first use

uint32_t crc32_fast_mask(const UINT8* source, const UINT8* mask, size_t n); // computing a buffer CRC32 on the masked area, "build_crc32_table()" must have been called before the first use

#endif