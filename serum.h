#pragma once

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT;

enum 
{
	SERUM_V1,
	SERUM_V2
};


enum
{
	FLAG_REQUEST_32P_FRAMES = 1,				
	FLAG_REQUEST_64P_FRAMES = 2,				
	FLAG_REQUEST_FILL_MODIFIED_ELEMENTS = 4,	
};

enum 
{
	FLAG_RETURNED_32P_FRAME_OK = 1, 
	FLAG_RETURNED_64P_FRAME_OK = 2, 
};

enum 
{
	FLAG_RETURNED_V1_ROTATED = 0x10000,
	FLAG_RETURNED_V2_ROTATED32 = 0x10000,
	FLAG_RETURNED_V2_ROTATED64 = 0x20000,
};

typedef struct
{
	
	UINT8* frame; 
	UINT8* palette; 
	UINT8* rotations; 
	
	
	
	UINT16* frame32;
	UINT width32; 
	UINT16* rotations32;
	UINT16* rotationsinframe32; 
	UINT8* modifiedelements32; 
	UINT16* frame64;
	UINT width64; 
	UINT16* rotations64;
	UINT16* rotationsinframe64;  
	UINT8* modifiedelements64; 
	
	UINT SerumVersion; 
	
	
	
	
	
	
	
	UINT8 flags;
	unsigned int nocolors; 
	unsigned int ntriggers; 
	UINT triggerID; 
	UINT frameID; 
	UINT rotationtimer;
}Serum_Frame_Struc;

const int MAX_DYNA_4COLS_PER_FRAME = 16;  
const int MAX_DYNA_SETS_PER_FRAMEN = 32;  
const int MAX_SPRITE_SIZE = 128;  
const int MAX_SPRITE_WIDTH = 256; 
const int MAX_SPRITE_HEIGHT = 64; 
const int MAX_SPRITES_PER_FRAME = 32;  
const int MAX_COLOR_ROTATIONS = 8;  
const int MAX_COLOR_ROTATIONN = 4; 
const int MAX_LENGTH_COLOR_ROTATION = 64; 
const int MAX_SPRITE_DETECT_AREAS = 4;  

const int PALETTE_SIZE = 64 * 3;  
const int ROTATION_SIZE = 3 * MAX_COLOR_ROTATIONS;  
const int MAX_BACKGROUND_IMAGES = 255;  

typedef Serum_Frame_Struc* (*Serum_LoadFunc)(const char* const altcolorpath, const char* const romname, UINT8 flags);
typedef void (*Serum_DisposeFunc)(void);
typedef UINT (*Serum_ColorizeFunc)(UINT8* frame);
typedef UINT (*Serum_RotateFunc)(void);
typedef const char* (*Serum_GetVersionFunc)(void);
