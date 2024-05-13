#ifndef CRC32_H
#define CRC32_H

#include "targetver.h"
#include <stdint.h>
#include <basetsd.h>
#include <windows.h>

void build_crc32_table(void); 

uint32_t crc32_fast(const UINT8* s, size_t n, BOOL ShapeMode); 

uint32_t crc32_fast_step(const UINT8* s, const UINT step, size_t n, BOOL ShapeMode); 

uint32_t crc32_fast_count(const UINT8* s, size_t n, BOOL ShapeMode, UINT8* pncols); 


uint32_t crc32_fast_mask_shape(const UINT8* source, const UINT8* mask, size_t n, BOOL ShapeMode); 

uint32_t crc32_fast_mask(const UINT8* source, const UINT8* mask, size_t n); 

#endif