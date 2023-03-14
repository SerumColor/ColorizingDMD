#pragma once

#define MAX_GROUP_NUMBER 256

enum {
	BM_COLMODE = WM_USER,
    BM_NEW, 
    BM_OPEN,
    BM_SAVE,
    BM_ADD,
    BM_IMPORT,
    BM_EXPORT,
	BM_UNDO,
	BM_REDO,
	BM_SELALL,
    BM_SELMASK,
    BM_SELMOVMASK,
    BM_FILTER,
    BM_ADDTOMASK,
    BM_REMFROMMASK,
    BM_UNICMASK,
    BM_DELMASK,
    BM_MASKCOL0,
    BM_MASKCOL1,
    BM_MASKCOL2,
    BM_MASKCOL3,
    BM_MOVEPATTERN,
    BM_DRAWALL,
    BM_EDITPAL,
	BM_ORGMODE,
	BM_MASKMULTI
};
#define BM_DRAWCOL0 BM_MASKCOL0
#define BM_DRAWCOL1 BM_MASKCOL1
#define BM_DRAWCOL2 BM_MASKCOL2
#define BM_DRAWCOL3 BM_MASKCOL3

enum
{
	SA_DRAW = 0,
	SA_SELECTION,
	SA_COMPMASK,
	SA_PALETTE,
	SA_MASKCOLORS,
	SA_MOVCOMPPATTERN,
	SA_EDITCOLOR,
	SA_MASKID,
	SA_SHAPEMODE,
	SA_COLSETS,
	SA_DYNACOLOR,
	SA_COPYMASK,
	SA_DYNAMASK,
	SA_SPRITECOLOR,
	SA_ACSPRITE,
	SA_SPRITE,
	SA_SPRITES,
	//SA_ADDSPRITE,
	//SA_DELSPRITE,
	SA_FRAMESPRITES,
	SA_SECTIONS,
	SA_COLROT
};

#define MAX_DYNA_SETS_PER_FRAME 16 // max number of color sets for dynamic content for each frame
#define MAX_COMP_RECTS 255 // max number of comparison rects and moving rects
#define MAX_COL_SETS 64 // max number of 4-color sets to remember
#define MAX_MASKS 64 // max number of comparison masks for comparison
#define SIZE_MASK_NAME 32 // size of dyna mask names
#define MAX_UNDO 16 // maximum undo/redo actions
#define MAX_SECTIONS 512 // maximum number of frame sections
#define SIZE_SECTION_NAMES 32 // size of section names
#define TOOLBAR_HEIGHT 100 // ...
#define MAX_SPRITES_PER_FRAME 32 // maximum amount of sprites to look for per frame
#define MAX_SPRITE_SIZE 128 // maximum size of the sprites
#define MAX_SPRITE_SIZE2 64 // maximum size of the sprites
#define SKIP_FRAME_DURATION 15 // skip the frames that are shorter than SKIP_FRAME_DURATION ms
#define MAX_COLOR_ROTATION 8 // maximum number of color rotations per frame
#define MAX_SPRITE_DETECT_AREAS 4 // maximum number of areas to detect the sprite
#define DEFAULT_FRAME_DURATION 30 // if a frame duration can not be calculated set this value as default
#define MAX_FRAME_DURATION 4000 // maximum frame duration

typedef struct
{
	// header
	char		name[64]; // ROM name (no .zip, no path, example: afm_113b)
	UINT32		fWidth;	// Frame width=fW
	UINT32		fHeight;	// Frame height=fH
	UINT32		nFrames;	// Number of frames=nF
	UINT32		noColors;	// Number of colors in palette of original ROM=noC
	UINT32		ncColors;	// Number of colors in palette of colorized ROM=nC
	UINT32		nCompMasks; // Number of dynamic masks=nM
	UINT32		nMovMasks; // Number of moving rects=nMR
	UINT32		nSprites; // Number of sprites=nS (max 255)
	// data
	// part for comparison
	UINT32*		HashCode;	// UINT32[nF] hashcode/checksum
	UINT8*		CompMaskID;	// UINT8[nF] Comparison mask ID per frame (255 if no rectangle for this frame)
	UINT8*		ShapeCompMode;	// UINT8[nF] FALSE - full comparison (all 4 colors) TRUE - shape mode (we just compare black 0 against all the 3 other colors as if it was 1 color)
								// HashCode take into account the ShapeCompMode parameter converting any '2' or '3' into a '1'
	UINT8*		MovRctID;	// UINT8[nF] Horizontal moving comparison rectangle ID per frame (255 if no rectangle for this frame)
	UINT8*		CompMasks;	// UINT8[nM*fW*fH] Mask for comparison
	UINT8*		MovRcts; // UINT8[nMR*4] Rect for Moving Comparision rectangle [x,y,w,h]. The value (<MAX_DYNA_SETS_PER_FRAME) points to a sequence of 4 colors in Dyna4Cols. 255 means not a dynamic content.
	// part for colorization
	UINT8*		cPal;		// UINT8[3*nC*nF] Palette for each colorized frames
	UINT8*		cFrames;	// UINT8[nF*fW*fH] Colorized frames color indices, if this frame has sprites, it is the colorized frame of the static scene, with no sprite
	UINT8*		DynaMasks;	// UINT8[nF*fW*fH] Mask for dynamic content for each frame.  The value (<MAX_DYNA_SETS_PER_FRAME) points to a sequence of 4 colors in Dyna4Cols. 255 means not a dynamic content.
	UINT8*		Dyna4Cols;  // UINT8[nF*MAX_DYNA_SETS_PER_FRAME*noC] Color sets used to fill the dynamic content
	UINT8*		FrameSprites; // UINT8[nF*MAX_SPRITES_PER_FRAME] Sprite numbers to look for in this frame max=MAX_SPRITES_PER_FRAME 255 if no sprite
	UINT16*		SpriteDescriptions; // UINT16[nS*MAX_SPRITE_SIZE*MAX_SPRITE_SIZE] Sprite drawing on 2 bytes per pixel:
								    // - the first is the 4-or-16-color sprite original drawing (255 means this is a transparent=ignored pixel) for Comparison step
								    // - the second is the 64-color sprite for Colorization step
	UINT8*		ColorRotations; // UINT8[3*MAX_COLOR_ROTATION*nF] 3 bytes per rotation: 1- first color; 2- last color; 3- number of 10ms between two rotations
	UINT16*		SpriteDetAreas; // UINT16[nS*4*MAX_SPRITE_DETECT_AREAS] rectangles (left, top, width, height) as areas to detect sprites (left=0xffff -> no zone)
	UINT32*		SpriteDetDwords; // UINT32[nS*MAX_SPRITE_DETECT_AREAS] dword to quickly detect 4 consecutive distinctive pixels inside the original drawing of a sprite for optimized detection
	UINT16*		SpriteDetDwordPos; // UINT16[nS*MAX_SPRITE_DETECT_AREAS] offset of the above dword in the sprite description
	UINT32*		TriggerID; // UINT32[nF] does this frame triggers any event ID, 0xFFFFFFFF if not
}cRom_struct;

typedef struct
{
	// Header
	char		name[64]; // ROM name (no .zip, no path, example: afm_113b)
	UINT8*		oFrames;	// UINT8[nF*fW*fH] Original frames (TXT converted to byte '2'->2)
	BOOL		activeColSet[MAX_COL_SETS]; // 4-or-16-color sets active
	UINT8		ColSets[MAX_COL_SETS * 16]; // the 4-or-16-color sets
	UINT8		acColSet; // current 4-or-16-color set
	UINT8		preColSet; // first 4-or-16-color set displayed in the dialogbox
	char		nameColSet[MAX_COL_SETS * 64]; // caption of the colsets
	UINT32		DrawColMode; // 0- 1 col mode, 1- 4 color set mode, 2- gradient mode
	UINT8		Draw_Mode;	// in colorization mode: 0- point, 1- line, 2- rect, 3- circle, 4- fill
	int			Mask_Sel_Mode; // in comparison mode: 0- point, 1- rectangle, 2- magic wand
	BOOL		Fill_Mode; // FALSE- empty, TRUE- filled
	char		Mask_Names[MAX_MASKS * SIZE_MASK_NAME]; // the names of the synamic masks
	UINT32		nSections; // number of sections in the frames
	UINT32		Section_Firsts[MAX_SECTIONS]; // first frame of each section
	char		Section_Names[MAX_SECTIONS * SIZE_SECTION_NAMES]; // Names of the sections
	char		Sprite_Names[255 * SIZE_SECTION_NAMES]; // Names of the sprites
	UINT32		Sprite_Col_From_Frame[255]; // Which frame is used for the palette of the sprite colors
	UINT8		Sprite_Edit_Colors[16 * 255]; // Which color are used to edit this sprite
	UINT32*		FrameDuration; // UINT32[nF] duration of the frame
	char		SaveDir[260]; // char[260] save directory
	UINT16		SpriteRect[255*4]; // UINT16[255*4] rectangle where to find the sprite in the frame Sprite_Col_From_Frame
	BOOL		SpriteRectMirror[255 * 2]; // BOOL[255*2] has the initial rectangle been mirrored horizontally or/and vertically
}cRP_struct;

typedef struct {
	BOOL active; // deactivated if similar to a previous one
	char* ptr; // pointer to the first color index of the frame inside the TXT file
	UINT32 timecode; // time code of the frame
	UINT32 hashcode; // hashcode
} sFrames;

typedef struct {
	DWORD lastTime[MAX_COLOR_ROTATION]; // for C# we'll use DateTime and TimeSpan https://dotnetcodr.com/2016/10/20/two-ways-to-measure-time-in-c-net/
	UINT8 firstcol[MAX_COLOR_ROTATION]; // initial first color of the rotation, 255 if the rotation is not active
	UINT8 ncol[MAX_COLOR_ROTATION]; // number of colors in the rotation
	UINT8 acfirst[MAX_COLOR_ROTATION]; // current offset of the colors
	DWORD timespan[MAX_COLOR_ROTATION]; // time span between 2 switches
} sColRot;