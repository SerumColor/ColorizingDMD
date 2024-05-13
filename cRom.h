#pragma once

#include "serum.h"

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
	SA_SPRITEDRAW,
	SA_SELECTION,
	SA_COMPMASK,
	SA_PALETTE,
	SA_DRAWPAL,
	SA_MASKCOLORS,
	SA_MOVCOMPPATTERN,
	
	SA_FULLDRAW,
	SA_MASKID,
	SA_SHAPEMODE,
	SA_COLSETS,
	SA_DYNACOLOR,
	SA_DYNAALL,
	SA_PALDYNACOLOR,
	SA_COPYMASK,
	SA_DYNAMASK,
	SA_ACCOLORS,
	SA_ACSPRITE,
	SA_SPRITE,
	SA_SPRITES,
	SA_FRAMESPRITES,
	SA_SPRITEDETAREAS,
	SA_SECTIONS,
	SA_COLROT,
	SA_DURATION,
	SA_SETBACKGROUND,
	SA_COPYBACKGROUND,
	SA_ADDBACKGROUND,
	SA_SELBACKGROUND,
	SA_DELBACKGROUND,
	SA_DELFRAMEBG,
	SA_REIMPORTBACKGROUND,
	SA_FRAMES, 
	SA_ISFRAMEX,
	SA_ISSPRITEX,
	SA_ISBGX,
	SA_TRIGGERID,
};


#define MAX_COMP_RECTS 255 
#define MAX_COL_SETS 64 
#define MAX_MASKS 64 
#define SIZE_MASK_NAME 32 
#define MAX_UNDO 1024 
#define MAX_SECTIONS 512 
#define SIZE_SECTION_NAMES 32 
#define TOOLBAR_HEIGHT 100 



#define SKIP_FRAME_DURATION 15 



#define DEFAULT_FRAME_DURATION 30 
#define MAX_FRAME_DURATION 4000 
#define MAX_SEL_FRAMES 1024 
#define MAX_BACKGROUNDS 255 
#define N_PALETTES 256 
#define N_IMAGE_POS_TO_SAVE 256 
typedef struct
{
	
	char		name[64]; 
	UINT32		fWidth;	
	UINT32		fHeight;	
	UINT32		fWidthX;	
	UINT32		fHeightX;	
	UINT32		nFrames;	
	UINT32		noColors;	

	UINT32		nCompMasks; 
	
	UINT32		nSprites; 
	UINT16		nBackgrounds; 
	
	
	UINT32*		HashCode;	
	UINT8*		ShapeCompMode;	
	UINT8*		CompMaskID;	
								
	
	UINT8*		CompMasks;	
	
	
	
	UINT8*		isExtraFrame;	
	UINT16*		cFrames;	
	UINT16*		cFramesX;	
	UINT8*		DynaMasks;	
	UINT8*		DynaMasksX;	
	UINT16*		Dyna4Cols;  
	UINT16*		Dyna4ColsX;  
	UINT8*		FrameSprites; 
	UINT16*		FrameSpriteBB; 
	
	
	
	UINT8*		isExtraSprite;	
	UINT8*		SpriteOriginal; 
	UINT16*		SpriteColored; 
	UINT8*		SpriteMaskX; 
	UINT16*		SpriteColoredX; 
	UINT16*		ColorRotations; 
	UINT16*		ColorRotationsX; 
	UINT16*		SpriteDetAreas; 
	UINT32*		SpriteDetDwords; 
	UINT16*		SpriteDetDwordPos; 
	UINT32*		TriggerID; 
	UINT8*		isExtraBackground;	
	UINT16*		BackgroundFrames; 
	UINT16*		BackgroundFramesX; 
	UINT16*		BackgroundID; 
	
	UINT8*		BackgroundMask; 
	UINT8*		BackgroundMaskX; 
}cRom_struct;

typedef struct
{
	
	char		name[64]; 
	UINT8*		oFrames;	
	BOOL		activeColSet[MAX_COL_SETS]; 
	UINT16		ColSets[MAX_COL_SETS * 16]; 
	UINT8		acColSet; 
	UINT8		preColSet; 
	char		nameColSet[MAX_COL_SETS * 64]; 
	UINT32		DrawColMode; 
	UINT8		Draw_Mode;	
	int			Mask_Sel_Mode; 
	BOOL		Fill_Mode; 
	char		Mask_Names[MAX_MASKS * SIZE_MASK_NAME]; 
	UINT32		nSections; 
	UINT32		Section_Firsts[MAX_SECTIONS]; 
	char		Section_Names[MAX_SECTIONS * SIZE_SECTION_NAMES]; 
	char		Sprite_Names[255 * SIZE_SECTION_NAMES]; 
	UINT32		Sprite_Col_From_Frame[255]; 
	
	UINT32*		FrameDuration; 
	
	UINT16		SpriteRect[255*4]; 
	BOOL		SpriteRectMirror[255 * 2]; 
	
	
	UINT16		Palette[N_PALETTES * 64]; 
	UINT16		acEditColorsS[16]; 
	char		PalNames[N_PALETTES * 64]; 
	UINT		nImagePosSaves; 
	char		ImagePosSaveName[N_IMAGE_POS_TO_SAVE * 64]; 
	int			ImagePosSave[N_IMAGE_POS_TO_SAVE * 16]; 
	UINT		isImported; 
	UINT8*		importedPal; 
}cRP_struct;

typedef struct {
	BOOL active; 
	char* ptr; 
	UINT32 timecode; 
	UINT32 hashcode; 
} sFrames;

