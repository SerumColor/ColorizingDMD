#pragma once

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned int UINT;

typedef struct
{
	// in former format (prior to 2.0.0) the returned frame replaces the original frame, so this is not
	// part of this 
	UINT8* frame; // return the colorized frame
	UINT8* palette; // and its palette
	UINT8* rotations; // and its color rotations
	UINT32* triggerID; // return 0xffff if no trigger for that frame, the ID of the trigger if one is set for that frame
	UINT* frameID; // for CDMD ingame tester
}Serum_Frame;

typedef struct
{
	// the frame (frame32 or frame64) corresponding to the original resolution must ALWAYS be defined
	// but the frame corresponding to the extra resolution must be defined only if we request it
	// if a frame is defined, its width, rotations and rotationsinframe must be defined
	UINT16* frame32;
	UINT* width32; // 0 is returned if the 32p colorized frame is not available for this frame
	UINT16* rotations32;
	UINT16* rotationsinframe32; // [(96 or 128)*32*2] precalculated array to tell if a color is in a color rotations of the frame ([X*Y*0]=0xffff if not part of a rotation)
	UINT16* frame64;
	UINT* width64; // 0 is returned if the 64p colorized frame is not available for this frame
	UINT16* rotations64;
	UINT16* rotationsinframe64;  // [(192 or 256)*64*2] precalculated array to tell if a color is in a color rotations of the frame ([X*Y*0]=0xffff if not part of a rotation)
	UINT32* triggerID; // return 0xffff if no trigger for that frame, the ID of the trigger if one is set for that frame
	UINT8* flags; // return flags:
	// if flags & 1 : frame32 has been filled
	// if flags & 2 : frame64 has been filled
	// if none of them, display the original frame
	UINT* frameID; // for CDMD ingame tester
}Serum_Frame_New;

const int MAX_DYNA_4COLS_PER_FRAME = 16;  // max number of color sets for dynamic content for each frame (old version)
const int MAX_DYNA_SETS_PER_FRAMEN = 32;  // max number of color sets for dynamic content for each frame (new version)
const int MAX_SPRITE_SIZE = 128;  // maximum size of the sprites
const int MAX_SPRITE_WIDTH = 256; // maximum width of the new sprites
const int MAX_SPRITE_HEIGHT = 64; // maximum height of the new sprites
const int MAX_SPRITES_PER_FRAME = 32;  // maximum amount of sprites to look for per frame
const int MAX_COLOR_ROTATIONS = 8;  // maximum amount of color rotations per frame
const int MAX_COLOR_ROTATIONN = 4; // maximum number of new color rotations per frame
const int MAX_LENGTH_COLOR_ROTATION = 64; // maximum number of new colors in a rotation
const int MAX_SPRITE_DETECT_AREAS = 4;  // maximum number of areas to detect the sprite

const int PALETTE_SIZE = 64 * 3;  // size of a palette
const int ROTATION_SIZE = 3 * MAX_COLOR_ROTATIONS;  // size of a color rotation block
const int MAX_SPRITE_TO_DETECT = 16;  // max number of sprites detected in a frame
const int MAX_BACKGROUND_IMAGES = 255;  // max number of background images

// Flags sent with Serum_Load
const int FLAG_REQUEST_32P_FRAMES = 1; // there is a output DMD which is 32 leds high
const int FLAG_REQUEST_64P_FRAMES = 2; // there is a output h is 64 leds high
const int FLAG_32P_FRAME_OK = 1; // the 32p frame has been filled
const int FLAG_64P_FRAME_OK = 2; // the 64p frame has been filled

typedef bool (*Serum_LoadFunc)(const char* const altcolorpath, const char* const romname, unsigned int* pnocolors, unsigned int* pntriggers, unsigned char flags, unsigned int* width32, unsigned int* width64, UINT8* isnewformat);
typedef void (*Serum_DisposeFunc)(void);
typedef bool (*Serum_ColorizeFunc)(UINT8* frame, Serum_Frame* poldframe, Serum_Frame_New* pnewframe);
typedef bool (*Serum_ApplyRotationsFunc)(Serum_Frame* poldframe);
typedef bool (*Serum_ApplyRotationsNFunc)(Serum_Frame_New* pnewframe, UINT8* modelements32, UINT8* modelements64);
