/*
* ColorizingDMD: Program to edit cROM colorized roms
* Programmed in plain C with Visual Studio 2022 by Zedrummer, 2022
* 
* Linked to the project Visual Pinball Engine for Unity and, as such, is licensed under the GNU General Public License v3.0 https://github.com/freezy/VisualPinball.Engine/blob/master/LICENSE
* 
* Uses OpenGL Immediate Mode for display in 2D in window as this is by far fast enough to make it works at 100+ FPS even on a low end computer with any dedicated GPU
* 
*/

#pragma region Includes

#include "framework.h"
#include "ColorizingDMD.h"
#include <strsafe.h>
#include <gdiplus.h>
using namespace Gdiplus;
#include <CommCtrl.h>
#include "resource.h"
#include <shlwapi.h>
#include "cRom.h"
#include <tchar.h>
#include <direct.h>
#include "OGL_Immediate_2D.h"
#include <windowsx.h>
#include <math.h>
#include <shlobj_core.h>
#include <crtdbg.h>
#include "LiteZip.h"

#pragma endregion Includes

#pragma region Global_Variables

#define MAJ_VERSION 1
#define MIN_VERSION 21
#define PATCH_VERSION 2

static TCHAR szWindowClass[] = _T("ColorizingDMD");
static TCHAR szWindowClass2[] = _T("ChildWin");

HINSTANCE hInst;                                // current instance
HWND hWnd = NULL, hwTB = NULL, hwTB2 = NULL, hPal = NULL, hPal2 = NULL, hPal3 = NULL, hMovSec = NULL, hColSet = NULL, hSprites = NULL, hStatus = NULL, hStatus2 = NULL;
UINT ColSetMode = 0, preColRot = 0, acColRot = 0; // 0- displaying colsets, 1- displaying colrots
GLFWwindow * glfwframe, * glfwframestrip,*glfwsprites,*glfwspritestrip;	// handle+context of our window
bool fDone = false;
HACCEL hAccelTable;
bool Update_Toolbar = false; // Do we need to recreate toolbar?
bool Update_Toolbar2 = false; // Do we need to recreate toolbar?

UINT8 SelFrameColor = 0; // variables for changing color frames
DWORD timeSelFrame = 0;

UINT Edit_Mode = 0; // 0- editing original frame, 1- editing colorized frame
UINT Comparison_Mode = 0; // 0- no mask mode, 1- exclusion mask mode, 2- horizontal moving rectangle inclusion mask
bool isLoadedProject = false; // is there a project loaded?
HIMAGELIST g_hImageList = NULL, g_hImageListD = NULL;
bool Night_Mode = false;

char DumpDir[MAX_PATH] = "D:\\visual pinball\\VPinMame\\dmddump\\";
bool Ask_for_SaveDir = true;

cRom_struct MycRom = { "",0,0,0,0,0,NULL,NULL,NULL,NULL,NULL,NULL,NULL };
cRP_struct MycRP = { "",{FALSE},{0},0,0,{0},FALSE,0,FALSE };

COLORREF PrevColors[16]={0};
UINT8 originalcolors[16 * 3];
UINT8 acEditColors[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
UINT noColSel = 0; // # color selected either for the mask color (original mode) or for the drawing color (colorized mode)
UINT noSprSel = 0; // # color selected for drawing the sprite
UINT noColMod = 0; // # color clicked for modification
UINT8 Draw_Extra_Surface[256 * 64]; // surface to draw over the frames (show circle, rectangle, etc... before they are really drawn to the fram)
UINT8 Draw_Extra_Surface2[MAX_SPRITE_SIZE * MAX_SPRITE_SIZE]; // surface to draw over the frames (show circle, rectangle, etc... before they are really drawn to the fram)
//UINT8 acDynaColSet = 0; // current 4-color set in Dyna4Cols used for dynamic content
bool Color_Pipette = false; // are we picking the color?

int frame_zoom = 1; // zoom multiplier to display the frame
int offset_frame_x, offset_frame_y; // offset in pixels of the top left corner of the frame in the client area
int sprite_zoom = 1; // zoom multiplier to display the sprite
int offset_sprite_x, offset_sprite_y; // offset in pixels of the top left corner of the sprite in the client area
UINT TxCircle = (UINT) - 1; // LED imitation circle texture ID
UINT TxFrameStrip[2] = { (UINT)-1, (UINT)-1 }; // Framebuffer texture for the strip displaying the frames ID
UINT TxSpriteStrip[2] = { (UINT)-1, (UINT)-1 }; // Framebuffer texture for the strip displaying the sprites ID
UINT TxChiffres, TxcRom; // Number texture ID
UINT acFSText = 0; // current texture displayed on the frame strip
UINT acSSText = 0; // current texture displayed on the sprite strip
UINT8 Raw_Digit_Def[RAW_DIGIT_W * RAW_DIGIT_H * 10]; // buffer for raw numbers
UINT8* pFrameStrip = NULL; // pointer to the memory to draw the frame strips
UINT8* pSpriteStrip = NULL; // pointer to the memory to draw the sprite strips
UINT SliderWidth, PosSlider; // Width of the frame strip slider and pos of the slider on the bar

UINT ScrW, ScrH; // client size of the main window
UINT ScrW2, ScrH2; // client size of the sprite window
int MonWidth = 1920; // X resolution of the monitor

int PreFrameInStrip = 0; // First frame to display in the frame strip
UINT NFrameToDraw = 0; // how many frames to draw in the strip
UINT FS_LMargin = 0;    // left margin (same as right) before the first frame in the strip

int PreSpriteInStrip = 0; // First sprite to display in the frame strip
UINT NSpriteToDraw = 0; // how many sprites to draw in the strip
UINT SS_LMargin = 0;    // left margin (same as right) before the first sprite in the strip

UINT acFrame = 0, prevFrame = 0; // current frame displayed and previously selected frame
UINT acSprite = 0, prevSprite=0; // current sprite displayed
UINT acDetSprite = 0; // current detection zone active for the sprite
unsigned int nSelFrames = 1; // number of selected frames
#define MAX_SEL_FRAMES 1024 // Max number of selected frames (has consequences on the undo/redo buffer size!)
unsigned int SelFrames[MAX_SEL_FRAMES]={0}; // list of selected frames
const UINT8 SelColor[3] = { 100,150,255 };
const UINT8 acColor[3] = { 255,255,255 };
const UINT8 SameColor[3] = { 0,180,0 };
const UINT8 UnselColor[3] = { 255,50,50 };
const UINT8 SectionColor[3] = { 255,50,255 };

bool MultiSelectionActionWarning = false; // is there a messagebox each time to confirm an action on a multi selection?
bool isDrawAllSel = false; // do we draw on all the selected frames?
bool isMaskAllSel = false; // do we modify the masks of all the selected frames?

DWORD timeLPress = 0, timeRPress = 0, timeUPress = 0, timeDPress = 0; // timer to know how long the key has been pressed
UINT32 Mouse_Mode = 0; // 0- no mouse activity, 1 - drawing the comparison mask (point), 2 - drawing the comparison/detection area mask (rectangle or magic wand), 3 - drawing the colorized frame (point), 4 - drawing the colorized frame (others), 5 - drawing the dynamic mask (point), 6 - drawing the dynamic mask (others), 7 - drawing the copy mask (point), 8 - drawing the copy mask (others)
bool isDel_Mode; // are we deleting from draw/mask?
//bool MouseActionCancelled = false; // do we cancelled the edit action while mouse clicked?
int MouseIniPosx, MouseIniPosy; // position of the mouse when starting to draw
int MouseFinPosx, MouseFinPosy; // position of the mouse when moving to draw
int MouseFrSliderlx; // previous position on the slider
bool MouseFrSliderLPressed = false; // mouse pressed on the frame strip slider

UINT8* UndoSave; // [MAX_SEL_FRAMES * 256 * 64 * MAX_UNDO] ; // Buffer to save things for the undo 
int UndoAvailable = 0; // how many actions to undo available
UINT UndoAction[MAX_UNDO]; // To know which action to do in case of undo: see enum SA_XXXXXXXX in cRom.h
UINT8* RedoSave;// [MAX_SEL_FRAMES * 256 * 64 * MAX_UNDO] ; // Buffer to save things for the redo
int RedoAvailable = 0; // how many actions to redo available
UINT RedoAction[MAX_UNDO]; // To know which action to do in case of redo: see enum SA_XXXXXXXX in cRom.h

bool isAddToMask = true; // true if the selection is add to mask, false if the selection is removed from mask

HANDLE hStdout; // for log window

bool DrawCommonPoints = true; // Do we display the common points of the selected frames (in original frame mode)?

HWND hCBList = NULL; // the combobox list to subclass for right click

#define MAX_SAME_FRAMES 10000
int nSameFrames = 0;
int SameFrames[MAX_SAME_FRAMES];

bool UpdateFSneeded = false; // Do we need to update the frame strip
bool UpdateSSneeded = false; // Do we need to update the sprite strip
//bool UpdateFSneededwhenBreleased = false; // Do we need to update the frame strip after the mouse button has been released

UINT8 Sprite_Mode = 0; // sprite drawing mode: 0- point, 1- line, 2- rectangle, 3- circle, 4- fill
bool SpriteFill_Mode = false; // sprite drawing filled rectangle and circle

UINT8 acDynaSet = 0; // current dynamic color set used
UINT8 mselcol; // flashing color %

UINT8 Copy_Mask[256 * 64] = { 0 }; // mask for copy operations
UINT8 Copy_Col[256 * 64]; // colors of copy
UINT8 Copy_Colo[256 * 64]; // original colors of copy
UINT8 Copy_Dyna[256 * 64]; // dynamic mask for copy
UINT Paste_Width, Paste_Height; // Dimensions for paste
UINT8 Paste_Mask[256 * 64]; // mask copy of the copy for paste
UINT8 Paste_Col[256 * 64]; // colors of copy
UINT8 Paste_Dyna[256 * 64]; // dynamic mask for copy
int Copy_From_Frame = -1; // which frame to copy from
bool Copy_Mode = false; // do we add to copy mask?
bool Copy_Available = false; // Is there any content to paste?
int paste_offsetx, paste_offsety, paste_centerx, paste_centery; // position of the copy in the new frame, and position of the middle of the copied area
bool Paste_Mode = false; // are we waiting for a click in the frame to paste
int Paste_Mirror = 0; // are we flipping the copied area horizontally and/or vertically 0- no, 1- horizontally, 2- vertically, 3- both
HCURSOR hcColPick, hcArrow, hcPaste; // pick color cursor, standard arrow cursor, paste cursor
GLFWcursor *glfwpastecur, *glfwdropcur, *glfwarrowcur; // glfw cursors

bool Start_Gradient = false; // are we creating a color gradient?
UINT8 Ini_Gradient_Color, Fin_Gradient_Color; // what is the first and last colors for the gradient

bool Start_Gradient2 = false; // are we creating a color gradient for filling?
UINT8 Draw_Grad_Ini = 0, Draw_Grad_Fin = 63; // colors for draw in gradient

bool Start_Col_Exchange = false; // are we chganging the position of 2 colors in the palette
UINT8 Pre_Col_Exchange = 0; // first color selected for exchange

sColRot MyColRot; // color rotation structure

bool Ident_Pushed = false;

bool filter_time = false, filter_allmask = false, filter_color = false;
int filter_length = 15, filter_ncolor = 16;

int statusBarHeight;

#pragma endregion Global_Variables

#pragma region Undo_Tools

void EraseFirstSavedAction(bool isUndo)
{
    UINT* pAc;
    UINT8* pBuffer;
    if (isUndo)
    {
        pAc = UndoAction;
        pBuffer = UndoSave;
        UndoAvailable--;
    }
    else
    {
        pAc = RedoAction;
        pBuffer = RedoSave;
        RedoAvailable--;
    }
    for (UINT ti = 0; ti < MAX_UNDO - 1; ti++)
        pAc[ti] = pAc[ti + 1];
    memcpy(pBuffer, &pBuffer[MAX_SEL_FRAMES * 256 * 64], MAX_SEL_FRAMES * 256 * 64 * (MAX_UNDO - 1));
}

UINT8* SaveGetBuffer(bool isUndo)
{
    if (isUndo)
    {
        if (UndoAvailable == MAX_UNDO) EraseFirstSavedAction(true);
        return &UndoSave[MAX_SEL_FRAMES * 256 * 64 * UndoAvailable];
    }
    else
    {
        if (RedoAvailable == MAX_UNDO) EraseFirstSavedAction(false);
        return &RedoSave[MAX_SEL_FRAMES * 256 * 64 * RedoAvailable];
    }
}

void SaveSetAction(bool isUndo, UINT action)
{
    if (isUndo)
    {
        UndoAction[UndoAvailable] = action;
        UndoAvailable++;
        SendMessage(hwTB, TB_ENABLEBUTTON, BM_UNDO, MAKELONG(1, 0));
    }
    else
    {
        RedoAction[RedoAvailable] = action;
        RedoAvailable++;
        SendMessage(hwTB, TB_ENABLEBUTTON, BM_REDO, MAKELONG(1, 0));
    }
}

UINT8* RecoverGetBuffer(bool isUndo)
{
    if (isUndo) return &UndoSave[MAX_SEL_FRAMES * 256 * 64 * (UndoAvailable - 1)];
    else return &RedoSave[MAX_SEL_FRAMES * 256 * 64 * (RedoAvailable - 1)];
}

void RecoverAdjustAction(bool isUndo)
{
    if (isUndo)
    {
        UndoAvailable--;
        if (UndoAvailable == 0) SendMessage(hwTB, TB_ENABLEBUTTON, BM_UNDO, MAKELONG(0, 0));
    }
    else
    {
        RedoAvailable--;
        if (RedoAvailable == 0) SendMessage(hwTB, TB_ENABLEBUTTON, BM_REDO, MAKELONG(0, 0));
    }
}

void SaveDrawFrames(bool isUndo)
{
    // Save selected frame content for undo or redo action before drawing
    UINT8* pBuffer= SaveGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) memcpy(&pBuffer[ti * MycRom.fWidth * MycRom.fHeight],
        &MycRom.cFrames[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight);
    SaveSetAction(isUndo, SA_DRAW);
}

void RecoverDrawFrames(bool isUndo)
{
    // Undo or redo a saved frame before drawing
    UINT8* pBuffer= RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) memcpy(&MycRom.cFrames[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight],
       &pBuffer[ti * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight);
    RecoverAdjustAction(isUndo);
}

void SaveSelection(bool isUndo)
{
    // Save selected frame list for undo or redo action before changing it
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    *((unsigned int*)pBuffer) = nSelFrames;
    *((UINT*)(&pBuffer[4])) = acFrame;
    memcpy(&pBuffer[8], SelFrames, nSelFrames * sizeof(int));
    SaveSetAction(isUndo, SA_SELECTION);
}

void RecoverSelection(bool isUndo)
{
    // Undo or redo a saved selection
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    nSelFrames = *((unsigned int*)pBuffer);
    acFrame = *((UINT*)(&pBuffer[4]));
    InitColorRotation();
    if (acFrame >= PreFrameInStrip + NFrameToDraw) PreFrameInStrip = acFrame - NFrameToDraw + 1;
    if ((int)acFrame < PreFrameInStrip) PreFrameInStrip = acFrame;
    memcpy(SelFrames, &pBuffer[8], nSelFrames * sizeof(int));
    UpdateNewacFrame();
    RecoverAdjustAction(isUndo);
}

void SaveDynaColor(bool isUndo)
{
    // Save selected frame list for undo or redo action before changing it
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) memcpy(&pBuffer[ti * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors],
        &MycRom.Dyna4Cols[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors], MAX_DYNA_SETS_PER_FRAME * MycRom.noColors);
    SaveSetAction(isUndo, SA_DYNACOLOR);
}

void RecoverDynaColor(bool isUndo)
{
    // Undo or redo a saved selection
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) memcpy(&MycRom.Dyna4Cols[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors],
        &pBuffer[ti * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors], MAX_DYNA_SETS_PER_FRAME * MycRom.noColors);
    RecoverAdjustAction(isUndo);
}

void SaveCompMask(bool isUndo)
{
    // Save selected frame content for undo or redo action before drawing
    UINT8 ti = MycRom.CompMaskID[acFrame];
    if (ti == 255) return;
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    memcpy(pBuffer, &MycRom.CompMasks[ti * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight);
    SaveSetAction(isUndo, SA_COMPMASK);
}

void RecoverCompMask(bool isUndo)
{
    // Undo or redo a saved frame before drawing
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    UINT8 ti = MycRom.CompMaskID[acFrame];
    memcpy(&MycRom.CompMasks[ti * MycRom.fWidth * MycRom.fHeight], pBuffer, MycRom.fWidth * MycRom.fHeight);
    RecoverAdjustAction(isUndo);
}

void SavePalette(bool isUndo)
{
    // Save selected frame content for undo or redo action before drawing
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) memcpy(&pBuffer[ti * MycRom.ncColors * 3],
        &MycRom.cPal[SelFrames[ti] * MycRom.ncColors * 3], MycRom.ncColors * 3);
    SaveSetAction(isUndo, SA_PALETTE);
}

void RecoverPalette(bool isUndo)
{
    // Undo or redo a saved frame before drawing
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) memcpy(&MycRom.cPal[SelFrames[ti] * MycRom.ncColors * 3],
        &pBuffer[ti * MycRom.ncColors * 3], MycRom.ncColors * 3);
    for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
    for (UINT ti = IDC_DYNACOL1; ti <= IDC_DYNACOL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
    RecoverAdjustAction(isUndo);
}

void SaveMaskID(bool isUndo)
{
    // Save selected frame content for undo or redo action before drawing
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) pBuffer[ti] = MycRom.CompMaskID[SelFrames[ti]];
    SaveSetAction(isUndo, SA_MASKID);
}

void RecoverMaskID(bool isUndo)
{
    // Undo or redo a saved frame before drawing
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) MycRom.CompMaskID[SelFrames[ti]] = pBuffer[ti];
    UpdateMaskList();
    CheckSameFrames();
    RecoverAdjustAction(isUndo);
}

void SaveShapeMode(bool isUndo)
{
    // Save selected frame content for undo or redo action before drawing
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) pBuffer[ti] = MycRom.ShapeCompMode[SelFrames[ti]];
    SaveSetAction(isUndo, SA_SHAPEMODE);
}

void RecoverShapeMode(bool isUndo)
{
    // Undo or redo a saved frame before drawing
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++) MycRom.ShapeCompMode[SelFrames[ti]] = pBuffer[ti];
    if (MycRom.ShapeCompMode[acFrame] == TRUE) Button_SetCheck(GetDlgItem(hwTB, IDC_SHAPEMODE), BST_CHECKED); else Button_SetCheck(GetDlgItem(hwTB, IDC_SHAPEMODE), BST_UNCHECKED);
    CheckSameFrames();
    RecoverAdjustAction(isUndo);
}

void SaveColorSets(bool isUndo)
{
    // Save selected frame content for undo or redo action before drawing
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    memcpy(pBuffer, MycRP.ColSets, MAX_COL_SETS * 16); // copy the col sets
    memcpy(&pBuffer[MAX_COL_SETS * 16], MycRP.nameColSet, MAX_COL_SETS * 64); // copy their names
    memcpy(&pBuffer[MAX_COL_SETS * (16 + 64)], MycRP.activeColSet,MAX_COL_SETS); // copy the active one
    SaveSetAction(isUndo, SA_SHAPEMODE);
}

void RecoverColorSets(bool isUndo)
{
    // Undo or redo a saved frame before drawing
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    memcpy(MycRP.ColSets, pBuffer, MAX_COL_SETS * 16);
    memcpy(MycRP.nameColSet, &pBuffer[MAX_COL_SETS * 16], MAX_COL_SETS * 64);
    memcpy(MycRP.activeColSet, &pBuffer[MAX_COL_SETS * (16 + 64)], MAX_COL_SETS);
    RecoverAdjustAction(isUndo);
}

void SaveEditColor(bool isUndo)
{
    // Save selected frame content for undo or redo action before drawing
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    memcpy(pBuffer, acEditColors, 16);
    SaveSetAction(isUndo, SA_EDITCOLOR);
}

void RecoverEditColor(bool isUndo)
{
    // Undo or redo a saved frame before drawing
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    memcpy(acEditColors, pBuffer, 16);
    if (Edit_Mode == 1)
    {
        for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
    }
    RecoverAdjustAction(isUndo);
}

void SaveCopyMask(bool isUndo)
{
    // Save selected frame content for undo or redo action before drawing
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    memcpy(pBuffer, Copy_Mask, 256 * 64);
    memcpy(&pBuffer[256 * 64], Copy_Col, 256 * 64);
    memcpy(&pBuffer[2 * 256 * 64], Copy_Colo, 256 * 64);
    memcpy(&pBuffer[3 * 256 * 64], Copy_Dyna, 256 * 64);
    SaveSetAction(isUndo, SA_COPYMASK);
}

void RecoverCopyMask(bool isUndo)
{
    // Undo or redo a saved frame before drawing
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    memcpy(Copy_Mask, pBuffer, 256 * 64);
    memcpy(Copy_Col, &pBuffer[256 * 64], 256 * 64);
    memcpy(Copy_Colo, &pBuffer[2 * 256 * 64], 256 * 64);
    memcpy(Copy_Dyna, &pBuffer[3 * 256 * 64], 256 * 64);
    RecoverAdjustAction(isUndo);
}

void SaveDynaMask(bool isUndo)
{
    // Save selected frame content for undo or redo action before drawing
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    for (UINT32 ti = 0; ti < nSelFrames; ti++) memcpy(&pBuffer[ti * MycRom.fWidth * MycRom.fHeight],
        &MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight);
    SaveSetAction(isUndo, SA_DYNAMASK);
}

void RecoverDynaMask(bool isUndo)
{
    // Undo or redo a saved frame before drawing
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT32 ti = 0; ti < nSelFrames; ti++) memcpy(&MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], 
        &pBuffer[ti * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight);
    RecoverAdjustAction(isUndo);
}

void SaveSpriteCol(bool isUndo)
{
    // Save selected frame content for undo or redo action before drawing
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    memcpy(pBuffer, &MycRP.Sprite_Edit_Colors[acSprite * 16], 16);
    SaveSetAction(isUndo, SA_SPRITECOLOR);
}

void RecoverSpriteCol(bool isUndo)
{
    // Undo or redo a saved frame before drawing
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    memcpy(&MycRP.Sprite_Edit_Colors[acSprite * 16], pBuffer, 16);
    RecoverAdjustAction(isUndo);
}

void SaveFrameSprites(bool isUndo)
{
    // Save selected frame content for undo or redo action before drawing
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        memcpy(&pBuffer[ti * MAX_SPRITES_PER_FRAME], &MycRom.FrameSprites[SelFrames[ti] * MAX_SPRITES_PER_FRAME], MAX_SPRITES_PER_FRAME);
    }
    SaveSetAction(isUndo, SA_FRAMESPRITES);
}

void RecoverFrameSprites(bool isUndo)
{
    // Undo or redo a saved frame before drawing
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT ti = 0; ti < nSelFrames; ti++)
    {
        memcpy(&MycRom.FrameSprites[SelFrames[ti] * MAX_SPRITES_PER_FRAME], &pBuffer[ti * MAX_SPRITES_PER_FRAME], MAX_SPRITES_PER_FRAME);
    }
    RecoverAdjustAction(isUndo);
}

void SaveAcSprite(bool isUndo)
{
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    *((UINT*)pBuffer) = acSprite;
    SaveSetAction(isUndo, SA_ACSPRITE);
}

void RecoverAcSprite(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    acSprite = *((UINT*)pBuffer);
    if (acSprite >= MycRom.nSprites) acSprite = MycRom.nSprites - 1;
    RecoverAdjustAction(isUndo);
}

void SaveSprite(bool isUndo)
{
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    memcpy(pBuffer, &MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE], MAX_SPRITE_SIZE * MAX_SPRITE_SIZE * sizeof(UINT16));
    memcpy(&pBuffer[MAX_SPRITE_SIZE * MAX_SPRITE_SIZE * sizeof(UINT16)], &MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS], 4 * MAX_SPRITE_DETECT_AREAS * sizeof(UINT16));
    SaveSetAction(isUndo, SA_SPRITE);
}

void RecoverSprite(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    memcpy(&MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE], pBuffer, MAX_SPRITE_SIZE * MAX_SPRITE_SIZE * sizeof(UINT16));
    memcpy(&MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS], &pBuffer[MAX_SPRITE_SIZE * MAX_SPRITE_SIZE * sizeof(UINT16)], 4 * MAX_SPRITE_DETECT_AREAS * sizeof(UINT16));
    RecoverAdjustAction(isUndo);
}

void SaveColRot(bool isUndo)
{
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    for (UINT32 ti = 0; ti < nSelFrames; ti++) memcpy(pBuffer, &MycRom.ColorRotations[SelFrames[ti] * 3 * MAX_COLOR_ROTATION], 3 * MAX_COLOR_ROTATION);
    SaveSetAction(isUndo, SA_COLROT);
}

void RecoverColRot(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    for (UINT32 ti = 0; ti < nSelFrames; ti++) memcpy(&MycRom.ColorRotations[SelFrames[ti] * 3 * MAX_COLOR_ROTATION], pBuffer, 3 * MAX_COLOR_ROTATION);
    RecoverAdjustAction(isUndo);
}

void SaveSprites(bool isUndo)
{
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    ((UINT32*)pBuffer)[0] = MycRom.nSprites;
    memcpy(&pBuffer[sizeof(UINT32)], MycRom.SpriteDescriptions, sizeof(UINT16) * MycRom.nSprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
    memcpy(&pBuffer[sizeof(UINT32) + sizeof(UINT16) * MycRom.nSprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE], MycRom.SpriteDetAreas, sizeof(UINT16) * MycRom.nSprites * 4 * MAX_SPRITE_DETECT_AREAS);
    SaveSetAction(isUndo, SA_SPRITES);
}

void RecoverSprites(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    UINT32 tns = MycRom.nSprites;
    MycRom.nSprites = ((UINT32*)pBuffer)[0];
    if (tns != MycRom.nSprites)
    {
        MycRom.SpriteDescriptions = (UINT16*)realloc(MycRom.SpriteDescriptions, sizeof(UINT16) * MycRom.nSprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
        MycRom.SpriteDetAreas = (UINT16*)realloc(MycRom.SpriteDetAreas, sizeof(UINT16) * MycRom.nSprites * 4 * MAX_SPRITE_DETECT_AREAS);
    }
    memcpy(MycRom.SpriteDescriptions, &pBuffer[sizeof(UINT32)], sizeof(UINT16) * MycRom.nSprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
    memcpy(MycRom.SpriteDetAreas, &pBuffer[sizeof(UINT32) + sizeof(UINT16) * MycRom.nSprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE], sizeof(UINT16) * MycRom.nSprites * 4 * MAX_SPRITE_DETECT_AREAS);
    UpdateSpriteList();
    InvalidateRect(hwTB2, NULL, TRUE);
    if (acSprite >= MycRom.nSprites) acSprite = MycRom.nSprites - 1;
}

void SaveSections(bool isUndo)
{
    UINT8* pBuffer = SaveGetBuffer(isUndo);
    *((UINT32*)pBuffer) = MycRP.nSections;
    memcpy(&pBuffer[4], MycRP.Section_Names, MAX_SECTIONS * SIZE_SECTION_NAMES);
    memcpy(&pBuffer[4 + MAX_SECTIONS * SIZE_SECTION_NAMES], MycRP.Section_Firsts, sizeof(UINT32) * MAX_SECTIONS);
    SaveSetAction(isUndo, SA_SECTIONS);
}

void RecoverSections(bool isUndo)
{
    UINT8* pBuffer = RecoverGetBuffer(isUndo);
    MycRP.nSections = *((UINT32*)pBuffer);
    memcpy(MycRP.Section_Names, &pBuffer[4], MAX_SECTIONS * SIZE_SECTION_NAMES);
    memcpy(MycRP.Section_Firsts, &pBuffer[4 + MAX_SECTIONS * SIZE_SECTION_NAMES], sizeof(UINT32) * MAX_SECTIONS);
    RecoverAdjustAction(isUndo);
    UpdateSectionList();
}

void SaveAction(bool isUndo, int action)
{
    // 0- draw to frames, 1- change selection, 2- change comp mask shape, 3- change palette, 4- change mask colors, 5- change moving comparison pattern
    switch (action)
    {
    case SA_DRAW: // save before drawing
        SaveDrawFrames(isUndo);
        break;
    case SA_SELECTION: // save before changing selection
        SaveSelection(isUndo);
        break;
    case SA_COMPMASK: // save before changing comp mask
       SaveCompMask(isUndo);
        break;
    case SA_PALETTE: // save before changing palette
        SavePalette(isUndo);
        break;
    case SA_MOVCOMPPATTERN: // save before changing moving comparison pattern
 //       SaveMovCompPattern(isUndo);
    case SA_EDITCOLOR:
        SaveEditColor(isUndo);
        break;
    case SA_DYNACOLOR:
        SaveDynaColor(isUndo);
        break;
    case SA_MASKID:
        SaveMaskID(isUndo);
        break;
    case SA_SHAPEMODE:
        SaveShapeMode(isUndo);
        break;
    case SA_COLSETS:
        SaveColorSets(isUndo);
        break;
    case SA_COPYMASK:
        SaveCopyMask(isUndo);
        break;
    case SA_DYNAMASK:
        SaveDynaMask(isUndo);
        break;
    case SA_SPRITECOLOR:
        SaveSpriteCol(isUndo);
        break;
    case SA_ACSPRITE:
        SaveAcSprite(isUndo);
        break;
    case SA_SPRITE:
        SaveSprite(isUndo);
        break;
    case SA_SPRITES:
        SaveSprites(isUndo);
        break;
    case SA_FRAMESPRITES:
        SaveFrameSprites(isUndo);
        break;
    case SA_SECTIONS:
        SaveSections(isUndo);
        break;
    case SA_COLROT:
        SaveColRot(isUndo);
        break;
    }
    UpdateFSneeded = true;
    UpdateSSneeded = true;
    if (UndoAvailable > 0)
    {
        EnableWindow(GetDlgItem(hwTB, IDC_UNDO), TRUE);
        EnableWindow(GetDlgItem(hwTB2, IDC_UNDO), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwTB, IDC_UNDO), FALSE);
        EnableWindow(GetDlgItem(hwTB2, IDC_UNDO), FALSE);
    }
    if (RedoAvailable > 0)
    {
        EnableWindow(GetDlgItem(hwTB, IDC_REDO), TRUE);
        EnableWindow(GetDlgItem(hwTB2, IDC_REDO), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwTB, IDC_REDO), FALSE);
        EnableWindow(GetDlgItem(hwTB2, IDC_REDO), FALSE);
    }
}

void RecoverAction(bool isUndo)
{
    int action;
    if (isUndo)
    {
        if (UndoAvailable == 0) return;
        action = UndoAction[UndoAvailable - 1];
    }
    else
    {
        if (RedoAvailable == 0) return;
        action = RedoAction[RedoAvailable - 1];
    }
    switch (action)
    {
    case SA_DRAW: // save before drawing
        SaveDrawFrames(!isUndo);
        RecoverDrawFrames(isUndo);
        break;
    case SA_SELECTION: // save before changing selection
        SaveSelection(!isUndo);
        RecoverSelection(isUndo);
        break;
    case SA_COMPMASK: // save before changing comp mask
        SaveCompMask(!isUndo);
        RecoverCompMask(isUndo);
        break;
    case SA_PALETTE: // save before changing palette
        SavePalette(!isUndo);
        RecoverPalette(isUndo);
        break;
    case SA_MOVCOMPPATTERN: // save before changing moving comparison pattern
        //SaveMovCompPattern(!isUndo);
        //RecoverMovCompPattern(isUndo);
        break;
    case SA_EDITCOLOR:
        SaveEditColor(!isUndo);
        RecoverEditColor(isUndo);
        break;
    case SA_DYNACOLOR:
        SaveDynaColor(!isUndo);
        RecoverDynaColor(isUndo);
        for (UINT ti = IDC_DYNACOL1; ti <= IDC_DYNACOL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
        break;
    case SA_MASKID:
        SaveMaskID(!isUndo);
        RecoverMaskID(isUndo);
        break;
    case SA_SHAPEMODE:
        SaveShapeMode(!isUndo);
        RecoverShapeMode(isUndo);
        break;
    case SA_COLSETS:
        SaveColorSets(!isUndo);
        RecoverColorSets(isUndo);
        break;
    case SA_COPYMASK:
        SaveCopyMask(!isUndo);
        RecoverCopyMask(isUndo);
        break;
    case SA_DYNAMASK:
        SaveDynaMask(!isUndo);
        RecoverDynaMask(isUndo);
        break;
    case SA_SPRITECOLOR:
        SaveSpriteCol(!isUndo);
        RecoverSpriteCol(isUndo);
        break;
    case SA_ACSPRITE:
        SaveAcSprite(!isUndo);
        RecoverAcSprite(isUndo);
        break;
    case SA_SPRITE:
        SaveSprite(!isUndo);
        RecoverSprite(isUndo);
        break;
    case SA_FRAMESPRITES:
        SaveFrameSprites(!isUndo);
        RecoverFrameSprites(isUndo);
        break;
    case SA_SPRITES:
        SaveSprites(!isUndo);
        RecoverSprites(isUndo);
        break;
    case SA_SECTIONS:
        SaveSections(!isUndo);
        RecoverSections(isUndo);
        break;
        SaveColRot(!isUndo);
        RecoverColRot(isUndo);
        break;
    }
    UpdateFSneeded = true;
    UpdateSSneeded = true;
    if (UndoAvailable > 0)
    {
        EnableWindow(GetDlgItem(hwTB, IDC_UNDO), TRUE);
        EnableWindow(GetDlgItem(hwTB2, IDC_UNDO), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwTB, IDC_UNDO), FALSE);
        EnableWindow(GetDlgItem(hwTB2, IDC_UNDO), FALSE);
    }
    if (RedoAvailable > 0)
    {
        EnableWindow(GetDlgItem(hwTB, IDC_REDO), TRUE);
        EnableWindow(GetDlgItem(hwTB2, IDC_REDO), TRUE);
    }
    else
    {
        EnableWindow(GetDlgItem(hwTB, IDC_REDO), FALSE);
        EnableWindow(GetDlgItem(hwTB2, IDC_REDO), FALSE);
    }
}

#pragma endregion Undo_Tools

#pragma region Debug_Tools

void cprintf(const char* format,...) // write to the console
{
    char tbuf[490];
    va_list argptr;
    va_start(argptr, format);
    vsprintf_s(tbuf,490, format, argptr);
    va_end(argptr);
    char tbuf2[512];
    SYSTEMTIME lt;
    GetLocalTime(&lt);
    sprintf_s(tbuf2, 512, "%02d:%02d: %s\n\r", lt.wHour,lt.wMinute, tbuf);
    WriteFile(hStdout, tbuf2, (DWORD)strlen(tbuf2), NULL, NULL);
}

void AffLastError(char* lpszFunction)
{
    // Retrieve the system error message for the last-error code

    char* lpMsgBuf;
    char* lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&lpMsgBuf,
        0, NULL);

    // Display the error message and exit the process

    lpDisplayBuf = (char*)LocalAlloc(LMEM_ZEROINIT,
        (strlen((LPCSTR)lpMsgBuf) + strlen((LPCSTR)lpszFunction) + 40) * sizeof(char));
    StringCchPrintfA(lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(char),
        "%s failed with error %d: %s",
        lpszFunction, dw, lpMsgBuf);
    cprintf(lpDisplayBuf);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

#pragma endregion Debug_Tools

#pragma region Memory_Tools

void Del_Buffer_Element(UINT8* pBuf, UINT* pnElt, UINT noElt, UINT Elt_Size)
{
    /* Erase the noElt-th element of the buffer pointed by pBuf.
    * Before the function, the buffer contains *pnElt element and each element is Elt_Size byte long.
    * Shift all the elements after noElt to the left and reduce the buffer size with realloc. *pnElt is decremetend at the end.*/
    if (noElt >= (*pnElt)) return;
    if ((*pnElt) == 1)
    {
        free(pBuf);
        pBuf = NULL;
        (*pnElt) = 0;
        return;
    }
    if (noElt < ((*pnElt) - 1)) memcpy(&pBuf[noElt * Elt_Size], &pBuf[(noElt + 1) * Elt_Size], ((*pnElt) - 1 - noElt) * Elt_Size);
    pBuf = (UINT8*)realloc(pBuf, ((*pnElt) - 1) * Elt_Size);
    (*pnElt)--;
}

bool Add_Buffer_Element(UINT8* pBuf, UINT* pnElt, UINT Elt_Size, UINT8* pSrc)
{
    /* Add an element to the buffer pointed by pBuf.
    * Before the function, the buffer contains *pnElt element and each element is Elt_Size byte long.
    * Increase the buffer size by Elt_Size bytes using realloc then copy the new element from pSrc copiing Elt_Size bytes at the end of the buffer.
    * if pSrc==NULL, fill the memory with 0s
    * *pnElt is incremented at the end*/
    pBuf = (UINT8*)realloc(pBuf, ((*pnElt) + 1) * Elt_Size);
    if (!pBuf)
    {
        cprintf("Unable to increase the buffer size in Add_Buffer_Element");
        return false;
    }
    if (pSrc != NULL) memcpy(&pBuf[(*pnElt) * Elt_Size], pSrc, Elt_Size); else memset(&pBuf[(*pnElt) * Elt_Size], 0, Elt_Size);
    (*pnElt)++;
    return true;
}

#pragma endregion Memory_Tools

#pragma region Color_Tools

void CopyColAndDynaCol(UINT fromframe, UINT toframe)
{
    for (UINT ti = 0; ti < MycRom.ncColors * 3; ti++)
    {
        MycRom.cPal[toframe * 3 * MycRom.ncColors + ti] = MycRom.cPal[fromframe * 3 * MycRom.ncColors + ti];
    }
    for (UINT ti = 0; ti < MAX_DYNA_SETS_PER_FRAME; ti++)
    {
        for (UINT tj = 0; tj < MycRom.noColors; tj++)
        {
            MycRom.Dyna4Cols[toframe * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors + ti * MycRom.noColors + tj] = MycRom.Dyna4Cols[fromframe * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors + ti * MycRom.noColors + tj];
        }
    }
}

bool Is_Used_Color(UINT nofr, UINT8 nocol)
{
    // check if a color is used in a frame cFrame or dynamasks
    for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
    {
        UINT8 quelset = MycRom.DynaMasks[nofr * MycRom.fWidth * MycRom.fHeight + ti];
        if (quelset == 255)
        {
            if (MycRom.cFrames[nofr * MycRom.fWidth * MycRom.fHeight + ti] == nocol) return true;
        }
        else
        {
            for (UINT tj = 0; tj < MycRom.noColors; tj++)
            {
                if (MycRom.Dyna4Cols[nofr * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors + MycRom.noColors * quelset + tj] == nocol) return true;
            }
        }
    }
    return false;
}

UINT8 Is_Color_in_Palette(UINT nofr, UINT8* colRGB)
{
    // check if a color is already present in the palette of a frame
    // colRGB is a pointer to the R, G and B values of the color
    // return the frame color number and 255 if no color corresponding
    for (UINT8 ti = 0; ti < MycRom.ncColors; ti++)
    {
        if ((MycRom.cPal[nofr * 3 * MycRom.ncColors + ti * 3] == colRGB[0]) &&
            (MycRom.cPal[nofr * 3 * MycRom.ncColors + ti * 3 + 1] == colRGB[1]) &&
            (MycRom.cPal[nofr * 3 * MycRom.ncColors + ti * 3 + 2] == colRGB[2]))
            return ti;
    }
    return (UINT8)255;
}

UINT8 Copy_New_Col_To_Palette(UINT nofr, UINT8* colRGB)
{
    // find an unused color in the palette of the frame and copy the RGB to that color
    // return the color number
    for (UINT8 ti = 0; ti < MycRom.ncColors; ti++)
    {
        if (!Is_Used_Color(nofr, ti))
        {
            MycRom.cPal[nofr * 3 * MycRom.ncColors + ti * 3] = colRGB[0];
            MycRom.cPal[nofr * 3 * MycRom.ncColors + ti * 3 + 1] = colRGB[1];
            MycRom.cPal[nofr * 3 * MycRom.ncColors + ti * 3 + 2] = colRGB[2];
            return ti;
        }
    }
    return 255;
}

void Choose_Color_Palette(int butid)
{
    RECT rc;
    GetWindowRect(GetDlgItem(hwTB, butid), &rc);
    if (hPal) DestroyWindow(hPal);
    if (hPal2) DestroyWindow(hPal2);
    if (hPal3) DestroyWindow(hPal3);
    hPal2 = hPal3 = NULL;
    if (butid < IDC_DYNACOL1) noColMod = butid - IDC_COL1; else noColMod = butid - IDC_DYNACOL1 + 16;
    hPal = CreateWindowEx(0, L"Palette", L"", WS_POPUP, rc.left, rc.top, 160 + 2 * MARGIN_PALETTE, 160 + 2 * MARGIN_PALETTE, NULL, NULL, hInst, NULL);       // Parent window.
    if (!hPal)
    {
        AffLastError((char*)"Create Pal Window");
        return;
    }
    ShowWindow(hPal, true);
}

void Choose_Color_Palette2(int butid)
{
    RECT rc;
    GetWindowRect(GetDlgItem(hwTB2, butid), &rc);
    if (hPal) DestroyWindow(hPal);
    if (hPal2) DestroyWindow(hPal2);
    if (hPal3) DestroyWindow(hPal3);
    hPal = hPal3 = NULL;
    noColMod = butid - IDC_COL1;
    hPal2 = CreateWindowEx(0, L"Palette", L"", WS_POPUP, rc.left, rc.top, 160 + 2 * MARGIN_PALETTE, 160 + 2 * MARGIN_PALETTE, NULL, NULL, hInst, NULL);       // Parent window.
    if (!hPal2)
    {
        AffLastError((char*)"Create Pal Window");
        return;
    }
    ShowWindow(hPal2, true);
}

void Choose_Color_Palette3(void)
{
    RECT rc;
    GetWindowRect(GetDlgItem(hwTB, IDC_COLROT), &rc);
    if (hPal) DestroyWindow(hPal);
    if (hPal2) DestroyWindow(hPal2);
    if (hPal3) DestroyWindow(hPal3);
    hPal2 = hPal = NULL;
    hPal3 = CreateWindowEx(0, L"Palette", L"", WS_POPUP, rc.left, rc.top, 160 + 2 * MARGIN_PALETTE, 160 + 2 * MARGIN_PALETTE, NULL, NULL, hInst, NULL);       // Parent window.
    if (!hPal3)
    {
        AffLastError((char*)"Create Pal Window");
        return;
    }
    ShowWindow(hPal3, true);
}

void Add_PrevColor(COLORREF color)
{
    // Add a color to the previous color list in the color picker dialog
    for (unsigned int ti = 0; ti < 15; ti++) PrevColors[ti] = PrevColors[ti + 1];
    PrevColors[15] = color;
}

bool ColorPicker(UINT8* pRGB,LONG xm,LONG ym)
{
    CHOOSECOLORA ChCol;
    ChCol.lStructSize = sizeof(CHOOSECOLORA);
    ChCol.hwndOwner = hWnd;
    ChCol.hInstance = NULL;
    ChCol.rgbResult = ((DWORD)pRGB[0]) + (((DWORD)pRGB[1]) << 8) + (((DWORD)pRGB[2]) << 16);
    ChCol.lpCustColors = PrevColors;
    ChCol.Flags =  CC_FULLOPEN | CC_RGBINIT;
    ChCol.lCustData = 0;
    ChCol.lpfnHook = NULL;
    ChCol.lpTemplateName = NULL;
    if (ChooseColorA(&ChCol) == TRUE)
    {
        SaveAction(true, SA_PALETTE);
        Add_PrevColor(ChCol.rgbResult);
        pRGB[0] = (UINT8)(ChCol.rgbResult & 0xff);
        pRGB[1] = (UINT8)((ChCol.rgbResult & 0xff00) >> 8);
        pRGB[2] = (UINT8)((ChCol.rgbResult & 0xff0000) >> 16);
        SetCursorPos(xm, ym);
        int butid;
        if (noColMod < 16) butid = IDC_COL1 + noColMod; else butid = IDC_DYNACOL1 + noColMod - 16;
        Choose_Color_Palette(butid);
        return true;
    }
    return false;
}

UINT8 draw_color[4],under_draw_color[4];

void SetRenderDrawColor(UINT8 R, UINT8 G, UINT8 B, UINT8 A)
{
    draw_color[0] = R;
    draw_color[1] = G;
    draw_color[2] = B;
    draw_color[3] = A;
}

void SetRenderDrawColorv(UINT8* col3, UINT8 A)
{
    draw_color[0] = col3[0];
    draw_color[1] = col3[1];
    draw_color[2] = col3[2];
    draw_color[3] = A;
}

void Init_oFrame_Palette(UINT8* pPal)
{
    // Initialize a colorized palette with default colors
    for (unsigned int ti = 0; ti < MycRom.noColors; ti++) // shader of noColors oranges
    {
        originalcolors[ti * 3] = pPal[ti * 3];
        originalcolors[ti * 3 + 1] = pPal[ti * 3 + 1];
        originalcolors[ti * 3 + 2] = pPal[ti * 3 + 2];
    }
}

void Init_cFrame_Palette(UINT8* pPal)
{
    // Initialize a colorized palette with default colors
    for (unsigned int ti = 0; ti < MycRom.noColors; ti++) // shader of noColors oranges
    {
        originalcolors[ti * 3] = pPal[ti * 3] = (UINT8)(255.0 * (float)ti / (float)(MycRom.noColors - 1));
        originalcolors[ti * 3 + 1] = pPal[ti * 3 + 1] = (UINT8)(127.0 * (float)ti / (float)(MycRom.noColors - 1));
        originalcolors[ti * 3 + 2] = pPal[ti * 3 + 2] = (UINT8)(0.0 * (float)ti / (float)(MycRom.noColors - 1));
    }
    if (MycRom.noColors < MycRom.ncColors) // then remaining colors with a shade of grey
    {
        for (unsigned int ti = MycRom.noColors; ti < MycRom.ncColors; ti++)
        {
            pPal[ti * 3] = (UINT8)(255.0 * (float)(ti - MycRom.noColors) / (float)(MycRom.ncColors - 1 - MycRom.noColors));
            pPal[ti * 3 + 1] = (UINT8)(255.0 * (float)(ti - MycRom.noColors) / (float)(MycRom.ncColors - 1 - MycRom.noColors));
            pPal[ti * 3 + 2] = (UINT8)(255.0 * (float)(ti - MycRom.noColors) / (float)(MycRom.ncColors - 1 - MycRom.noColors));
        }
    }
}

void Init_cFrame_Palette2(void)
{
    // Initialize a colorized palette with default colors
    for (unsigned int ti = 0; ti < MycRom.noColors; ti++) // shader of noColors oranges
    {
        originalcolors[ti * 3] = (UINT8)(255.0 * (float)ti / (float)(MycRom.noColors - 1));
        originalcolors[ti * 3 + 1] = (UINT8)(127.0 * (float)ti / (float)(MycRom.noColors - 1));
        originalcolors[ti * 3 + 2] = (UINT8)(0.0 * (float)ti / (float)(MycRom.noColors - 1));
    }
}

#pragma endregion Color_Tools

#pragma region Comparison_Tools

uint32_t crc32_table[256];

void build_crc32_table(void) // initiating the CRC table, must be called at startup
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

uint32_t crc32_fast(const UINT8* s, size_t n, BOOL ShapeMode) // computing a buffer CRC32, "build_crc32_table()" must have been called before the first use
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

uint32_t crc32_fast_count(const UINT8* s, size_t n, BOOL ShapeMode, UINT8* pncols) // computing a buffer CRC32, "build_crc32_table()" must have been called before the first use
// this version counts the number of different values found in the buffer
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

uint32_t crc32_fast_mask(const UINT8* source, const UINT8* mask, size_t n, BOOL ShapeMode) // computing a buffer CRC32 on the non-masked area, "build_crc32_table()" must have been called before the first use
// take into account if we are in shape mode
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

#pragma endregion Comparison_Tools

#pragma region Editor_Tools

/*void Paste_on_Frame(UINT nofr)
{
    for (UINT tj = 0; tj < MycRom.fHeight; tj++)
    {
        for (UINT ti = 0; ti < MycRom.fWidth; ti++)
        {
            UINT tx = ti + paste_offsetx, ty = tj + paste_offsety;
            if ((Paste_Mask[tj * MycRom.fWidth + ti] > 0) && (tx >= 0) && (tx < MycRom.fWidth) && (ty >= 0) && (ty < MycRom.fHeight))
            {
                MycRom.cFrames[nofr * MycRom.fWidth * MycRom.fHeight + ty * MycRom.fWidth + tx] = MycRom.cFrames[Copy_From_Frame * MycRom.fWidth * MycRom.fHeight + tj * MycRom.fWidth + ti];
            }
        }
    }
}*/

void Delete_Frame(UINT32 nofr)
{
    // as its name says
    if (nofr < MycRom.nFrames - 1)
    {
        int nfrdecal = MycRom.nFrames - nofr - 1;
        memcpy(&MycRom.HashCode[nofr], &MycRom.HashCode[nofr + 1], sizeof(UINT32) * nfrdecal);
        memcpy(&MycRom.CompMaskID[nofr], &MycRom.CompMaskID[nofr + 1], nfrdecal);
        memcpy(&MycRom.ShapeCompMode[nofr], &MycRom.ShapeCompMode[nofr + 1], nfrdecal);
        memcpy(&MycRom.MovRctID[nofr], &MycRom.MovRctID[nofr + 1], nfrdecal);
        UINT32 toffd = nofr * MycRom.fWidth * MycRom.fHeight;
        UINT32 toffs = (nofr + 1) * MycRom.fWidth * MycRom.fHeight;
        memcpy(&MycRP.oFrames[toffd], &MycRP.oFrames[toffs], nfrdecal * MycRom.fWidth * MycRom.fHeight);
        memcpy(&MycRom.cFrames[toffd], &MycRom.cFrames[toffs], nfrdecal * MycRom.fWidth * MycRom.fHeight);
        memcpy(&MycRom.DynaMasks[toffd], &MycRom.DynaMasks[toffs], nfrdecal * MycRom.fWidth * MycRom.fHeight);
        memcpy(&MycRom.cPal[nofr * 3 * MycRom.ncColors], &MycRom.cPal[(nofr + 1) * 3 * MycRom.ncColors], nfrdecal * 3 * MycRom.ncColors);
        memcpy(&MycRom.Dyna4Cols[nofr * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors], &MycRom.Dyna4Cols[(nofr + 1) * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors], nfrdecal * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors);
        memcpy(&MycRom.FrameSprites[nofr * MAX_SPRITES_PER_FRAME], &MycRom.FrameSprites[(nofr + 1) * MAX_SPRITES_PER_FRAME], MAX_SPRITES_PER_FRAME * nfrdecal);
        memcpy(&MycRom.ColorRotations[nofr * MAX_COLOR_ROTATION * 3], &MycRom.ColorRotations[(nofr + 1) * MAX_COLOR_ROTATION * 3], MAX_COLOR_ROTATION * 3 * nfrdecal);
        memcpy(&MycRom.TriggerID[nofr], &MycRom.TriggerID[nofr + 1], sizeof(UINT32) * nfrdecal);
        memcpy(&MycRP.FrameDuration[nofr], &MycRP.FrameDuration[nofr + 1], sizeof(UINT32) * nfrdecal);
    }
    for (UINT32 ti = 0; ti < nSelFrames; ti++)
    {
        if (SelFrames[ti] > nofr) SelFrames[ti]--;
    }
    Del_Selection_Frame(nofr);
    for (int ti = 0; ti < nSameFrames; ti++)
    {
        if (SameFrames[ti] > (int)nofr) SameFrames[ti]--;
    }
    Del_Same_Frame(nofr);
    for (int ti = 0; ti < (int)MycRP.nSections; ti++)
    {
        if (MycRP.Section_Firsts[ti] > (int)nofr) MycRP.Section_Firsts[ti]--;
    }
    Del_Section_Frame(nofr);
    for (UINT ti = 0; ti < MycRom.nSprites; ti++)
    {
        if (MycRP.Sprite_Col_From_Frame[ti] > nofr) MycRP.Sprite_Col_From_Frame[ti]--;
        else if (MycRP.Sprite_Col_From_Frame[ti] == nofr) MycRP.SpriteRect[ti * 4] = 0xffff;
    }
    MycRom.nFrames--;
    if ((acFrame > nofr) && (acFrame > 0))
    {
        acFrame--;
        InitColorRotation();
        if (!isFrameSelected2(acFrame))
        {
            nSelFrames = 1;
            SelFrames[0] = acFrame;
        }
        UpdateNewacFrame();
        UpdateFSneeded = true;
    }
    if ((PreFrameInStrip > (int)nofr) && (PreFrameInStrip > 0)) PreFrameInStrip--;
    MycRom.HashCode = (UINT32*)realloc(MycRom.HashCode, MycRom.nFrames * sizeof(UINT32));
    MycRom.CompMaskID = (UINT8*)realloc(MycRom.CompMaskID, MycRom.nFrames);
    MycRom.MovRctID = (UINT8*)realloc(MycRom.MovRctID, MycRom.nFrames);
    MycRP.oFrames = (UINT8*)realloc(MycRP.oFrames, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    MycRom.cFrames = (UINT8*)realloc(MycRom.cFrames, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    MycRom.DynaMasks = (UINT8*)realloc(MycRom.DynaMasks, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    MycRom.cPal = (UINT8*)realloc(MycRom.cPal, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    MycRom.Dyna4Cols = (UINT8*)realloc(MycRom.Dyna4Cols, MycRom.nFrames * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors);
    MycRom.FrameSprites = (UINT8*)realloc(MycRom.FrameSprites, MycRom.nFrames * MAX_SPRITES_PER_FRAME);
    MycRom.ColorRotations = (UINT8*)realloc(MycRom.ColorRotations, MycRom.nFrames * 3 * MAX_COLOR_ROTATION);
    MycRom.TriggerID = (UINT32*)realloc(MycRom.TriggerID, MycRom.nFrames * sizeof(UINT32));
    MycRP.FrameDuration = (UINT32*)realloc(MycRP.FrameDuration, MycRom.nFrames * sizeof(UINT32));
}

void InitColorRotation(void)
{
    if (MycRom.name[0] == 0) return;
    DWORD actime = timeGetTime();
    for (UINT ti = 0; ti < MAX_COLOR_ROTATION; ti++)
    {
        MyColRot.lastTime[ti] = actime;
        MyColRot.firstcol[ti] = MycRom.ColorRotations[acFrame * 3 * MAX_COLOR_ROTATION + ti * 3];
        MyColRot.ncol[ti] = MycRom.ColorRotations[acFrame * 3 * MAX_COLOR_ROTATION + ti * 3 + 1];
        MyColRot.acfirst[ti] = 0;
        MyColRot.timespan[ti] = 10 * MycRom.ColorRotations[acFrame * 3 * MAX_COLOR_ROTATION + ti * 3 + 2];
    }
}

void CheckSameFrames(void)
{
    UINT8* pmsk = &MycRom.CompMasks[MycRom.CompMaskID[acFrame] * MycRom.fWidth * MycRom.fHeight];
    if (MycRom.CompMaskID[acFrame] == 255) pmsk = NULL;
    nSameFrames = 0;
    UINT8* pfrm = &MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight];
    bool isshape = MycRom.ShapeCompMode[acFrame];
    for (UINT32 tk = 0; tk < MycRom.nFrames; tk++)
    {
        if (tk == acFrame) continue;
        bool samefr = true;
        UINT8* pfrm2 = &MycRP.oFrames[tk * MycRom.fWidth * MycRom.fHeight];
        for (UINT32 ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
        {
            if (pmsk)
            {
                if (pmsk[ti] > 0) continue;
            }
            if (!isshape)
            {
                if (pfrm[ti] != pfrm2[ti])
                {
                    samefr = false;
                    break;
                }
            }
            else // in shape mode there is only black and coloured
            {
                if (((pfrm[ti] == 0) && (pfrm2[ti] > 0)) || ((pfrm[ti] > 0) && (pfrm2[ti] == 0)))
                {
                    samefr = false;
                    break;
                }
            }
        }
        if (!samefr) continue;
        if (nSameFrames < MAX_SAME_FRAMES)
        {
            SameFrames[nSameFrames] = tk;
            nSameFrames++;
        }
    }
    char tbuf[10];
    if (nSameFrames == MAX_SAME_FRAMES) sprintf_s(tbuf, 9, ">=%i", MAX_SAME_FRAMES);
    else _itoa_s(nSameFrames, tbuf, 9, 10);
    SetDlgItemTextA(hwTB, IDC_SAMEFRAME, tbuf);
}

void Add_Surface_To_Mask(UINT8* Surface, bool isDel)
{
    if (MycRom.name[0] == 0) return;
    if (MycRom.CompMaskID[acFrame] == 255) return;
    for (UINT32 ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
    {
        if (Surface[ti] > 0)
        {
            if (!isDel)
                MycRom.CompMasks[MycRom.CompMaskID[acFrame] * MycRom.fWidth * MycRom.fHeight + ti] = 1;
            else
                MycRom.CompMasks[MycRom.CompMaskID[acFrame] * MycRom.fWidth * MycRom.fHeight + ti] = 0;
        }
    }
    CheckSameFrames();
}

void Add_Surface_To_Copy(UINT8* Surface, bool isDel)
{
    if (MycRom.name[0] == 0) return;
    for (UINT32 ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
    {
        if (Surface[ti] > 0)
        {
            if (!isDel) Copy_Mask[ti] = 1; else Copy_Mask[ti] = 0;
            // we copy the whole frame anyway in case we invert the mask later on
        }
        Copy_Col[ti] = MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti];
        Copy_Colo[ti] = MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti];
        Copy_Dyna[ti] = MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight + ti];
    }
}

void Add_Surface_To_Dyna(UINT8* Surface, bool isDel)
{
    if (MycRom.name[0] == 0) return;
    for (UINT32 ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
    {
        for (UINT tj = 0; tj < nSelFrames; tj++)
        {
            if (Surface[ti] > 0)
            {
                if (!isDel)
                    MycRom.DynaMasks[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti] = acDynaSet;
                else
                {
                    if (MycRom.DynaMasks[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti] < 255)
                    MycRom.cFrames[SelFrames[tj] * MycRom.fHeight * MycRom.fWidth + ti] =
                        MycRP.oFrames[SelFrames[tj] * MycRom.fHeight * MycRom.fWidth + ti];
                    MycRom.DynaMasks[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti] = 255;
                }
            }
        }
    }
}

void Deactivate_Draw_All_Sel(void)
{
    isDrawAllSel = false;
    isMaskAllSel = false;
    SendMessage(hwTB, TB_CHECKBUTTON, BM_DRAWALL, MAKELONG(0, 0));
}

void InitVariables(void)
{
    Edit_Mode = 0;
    Comparison_Mode = 0;
    for (UINT ti = 0; ti < 15; ti++) PrevColors[ti] = 0;
    for (UINT ti = 0; ti < 15; ti++) acEditColors[ti] = ti;
    noColSel = 0;
    PreFrameInStrip = 0;
    acFrame = 0;
    InitColorRotation();
    prevFrame = 0;
    nSelFrames = 1;
    SelFrames[0] = 0;
    MultiSelectionActionWarning = false;
    isDrawAllSel = false;
    isMaskAllSel = false;
    UndoAvailable = 0;
    RedoAvailable = 0;
}

void UpdateNewacFrame(void)
{
    if (MycRom.name[0] == 0) return;
    int ti = Which_Section(acFrame);
    SetDlgItemTextA(hwTB, IDC_SECTIONNAME, &MycRP.Section_Names[ti * SIZE_SECTION_NAMES]);
    if (Edit_Mode == 0)
    {
        SendMessage(GetDlgItem(hwTB, IDC_SECTIONLIST), CB_SETCURSEL, ti + 1, 0);
        UINT8 puc = MycRom.CompMaskID[acFrame];
        puc++;
        SendMessage(GetDlgItem(hwTB, IDC_MASKLIST), CB_SETCURSEL, (WPARAM)puc, 0);
        if (MycRom.ShapeCompMode[acFrame]) SendMessage(GetDlgItem(hwTB, IDC_SHAPEMODE), BM_SETCHECK, BST_CHECKED, 0);
        else SendMessage(GetDlgItem(hwTB, IDC_SHAPEMODE), BM_SETCHECK, BST_UNCHECKED, 0);
        CheckSameFrames();
    }
    else
    {
        UpdateFrameSpriteList();
        SendMessage(GetDlgItem(hwTB, IDC_SECTIONLIST2), CB_SETCURSEL, ti + 1, 0);
    }
    InvalidateRect(hwTB, NULL, FALSE);
}

int isFrameSelected(UINT noFr)   // return -1 if the frame is not selected, the position in the selection list if already selected
{
    for (unsigned int ti = 0; ti < nSelFrames; ti++)
    {
        if (SelFrames[ti] == noFr) return (int)ti;
    }
    return -1;
}

int isFrameSelected3(UINT noFr)   // return -1 if the frame is not selected, the position in the selection list if already selected
{
    if ((acFrame == noFr) && (nSelFrames > 0)) return -2;
    for (unsigned int ti = 0; ti < nSelFrames; ti++)
    {
        if (SelFrames[ti] == noFr) return (int)ti;
    }
    return -1;
}

bool isFrameSelected2(UINT noFr)   // return -1 if the frame is not selected, the position in the selection list if already selected
{
    for (unsigned int ti = 0; ti < nSelFrames; ti++)
    {
        if (SelFrames[ti] == noFr) return true;
    }
    return false;
}

void Add_Selection_Frame(UINT nofr)
{
    // add a frame to the selection (except if the selection list is full)
    if (nSelFrames == MAX_SEL_FRAMES)
    {
        acFrame = SelFrames[nSelFrames - 1];
        InitColorRotation();
        return;
    }
    if (isFrameSelected(nofr) == -1)
    {
        SelFrames[nSelFrames] = nofr;
        nSelFrames++;
    }
}

void Del_Selection_Frame(UINT nofr)
{
    // remove a frame from the selection
    int possel = isFrameSelected(nofr);
    if (possel == -1) return;
    if (possel < (int)nSelFrames - 1)
    {
        for (UINT ti = possel; ti < nSelFrames - 1; ti++) SelFrames[ti] = SelFrames[ti + 1];
    }
    nSelFrames--;
}

int isSameFrame(UINT noFr) // return -1 if the frame is not similar to the current frame, the position in the same frame list if similar
{
    for (int ti = 0; ti < nSameFrames; ti++)
    {
        if (SameFrames[ti] == noFr) return ti;
    }
    return -1;
}

void Del_Same_Frame(UINT nofr)
{
    // remove a frame from the selection
    int possame = isSameFrame(nofr);
    if (possame == -1) return;
    if (possame < (int)nSameFrames - 1)
    {
        for (int ti = possame; ti < nSameFrames - 1; ti++) SameFrames[ti] = SameFrames[ti + 1];
    }
    nSameFrames--;
}

int is_Section_First(UINT nofr)
{
    for (UINT ti = 0; ti < MycRP.nSections; ti++)
    {
        if (MycRP.Section_Firsts[ti] == nofr) return (int)ti;
    }
    return -1;
}

int Which_Section(UINT nofr)
{
    int tres = -1, tfirst = -1;
    for (int ti = 0; ti < (int)MycRP.nSections; ti++)
    {
        if (((int)MycRP.Section_Firsts[ti] > tfirst) && (MycRP.Section_Firsts[ti] <= nofr))
        {
            tfirst = (int)MycRP.Section_Firsts[ti];
            tres = ti;
        }
    }
    return tres;
}

void Delete_Section(int nosec)
{
    for (int ti = nosec; ti < (int)MycRP.nSections - 1; ti++)
    {
        MycRP.Section_Firsts[ti] = MycRP.Section_Firsts[ti+1];
        for (int tj = 0; tj < SIZE_SECTION_NAMES; tj++) MycRP.Section_Names[ti * SIZE_SECTION_NAMES + tj] = MycRP.Section_Names[(ti + 1) * SIZE_SECTION_NAMES + tj];
    }
    MycRP.nSections--;
}

void Delete_Sprite(int nospr)
{
    if (nospr < (int)MycRom.nSprites - 1)
    {
        memmove(&MycRom.SpriteDescriptions[nospr * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE], &MycRom.SpriteDescriptions[(nospr + 1) * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE], sizeof(UINT16) * (MycRom.nSprites - 1 - nospr) * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
        memmove(&MycRP.Sprite_Col_From_Frame[nospr], &MycRP.Sprite_Col_From_Frame[nospr + 1], sizeof(UINT32) * (MycRom.nSprites - 1 - nospr));
        memmove(&MycRP.Sprite_Edit_Colors[16 * nospr], &MycRP.Sprite_Edit_Colors[16 * (nospr + 1)], (MycRom.nSprites - 1 - nospr) * 16);
        memmove(&MycRP.Sprite_Names[SIZE_SECTION_NAMES * nospr], &MycRP.Sprite_Names[SIZE_SECTION_NAMES * (nospr + 1)], (MycRom.nSprites - 1 - nospr) * SIZE_SECTION_NAMES);
        memmove(&MycRom.SpriteDetDwordPos[nospr * MAX_SPRITE_DETECT_AREAS], &MycRom.SpriteDetDwordPos[(nospr + 1) * MAX_SPRITE_DETECT_AREAS], sizeof(UINT16) * (MycRom.nSprites - 1 - nospr) * MAX_SPRITE_DETECT_AREAS);
        memmove(&MycRom.SpriteDetDwords[nospr * MAX_SPRITE_DETECT_AREAS], &MycRom.SpriteDetDwords[(nospr + 1) * MAX_SPRITE_DETECT_AREAS], sizeof(UINT32) * (MycRom.nSprites - 1 - nospr) * MAX_SPRITE_DETECT_AREAS);
        memmove(&MycRom.SpriteDetAreas[nospr * MAX_SPRITE_DETECT_AREAS*4], &MycRom.SpriteDetAreas[(nospr + 1) * MAX_SPRITE_DETECT_AREAS*4], sizeof(UINT16) * (MycRom.nSprites - 1 - nospr) * MAX_SPRITE_DETECT_AREAS*4);
    }
    MycRom.nSprites--;
}

int Duplicate_Section_Name(char* name)
{
    // check if a section already has this name
    if (strcmp(name, "- None -") == 0) return 30000; // if we have the "- None -" name, we return as if it was used
    for (UINT ti = 0; ti < MycRP.nSections; ti++)
    {
        if (strcmp(name, &MycRP.Section_Names[ti * SIZE_SECTION_NAMES]) == 0) return ti;
    }
    return -1;
}

int Duplicate_Sprite_Name(char* name)
{
    // check if a section already has this name
    //if (strcmp(name, "- None -") == 0) return 30000; // if we have the "- None -" name, we return as if it was used
    for (UINT ti = 0; ti < MycRom.nSprites; ti++)
    {
        if (strcmp(name, &MycRP.Sprite_Names[ti * SIZE_SECTION_NAMES]) == 0) return ti;
    }
    return -1;
}

void Del_Section_Frame(UINT nofr)
{
    // Update the section first frames and names when deleting a frame
    int tsec = is_Section_First(nofr);
    // if nofr is the first frame of a section and the next frame is the first frame of the section, we can delete the section
    if ((tsec > -1) && (is_Section_First(nofr + 1) > -1)) Delete_Section(tsec);
}

void Get_Mask_Pos(UINT x, UINT y, UINT* poffset, UINT8* pMask)
{
    // get pointer and mask value according the coordinates (x,y)
    *poffset = y * MycRom.fWidth / 8 + x / 8;
    *pMask = 0x80 >> (x % 8);
}

#pragma endregion Editor_Tools

#pragma region Window_Tools_And_Drawings

void Draw_Rectangle(GLFWwindow* glfwin, int ix, int iy, int fx, int fy)
{
    glfwMakeContextCurrent(glfwin);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);
    glBegin(GL_LINE_LOOP);
    glColor4ubv(draw_color);
    glVertex2i(ix, iy);
    glVertex2i(fx, iy);
    glVertex2i(fx, fy);
    glVertex2i(ix, fy);
    glEnd();
}

void Draw_Fill_Rectangle_Text(GLFWwindow* glfwin, int ix, int iy, int fx, int fy, UINT textID, float tx0, float tx1, float ty0, float ty1)
{
    glfwMakeContextCurrent(glfwin);
    glColor4ub(255, 255, 255, 255);
    glBindTexture(GL_TEXTURE_2D, textID);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBegin(GL_TRIANGLES);
    glTexCoord2f(tx0, ty0);
    glVertex2i(ix, iy);
    glTexCoord2f(tx1, ty0);
    glVertex2i(fx, iy);
    glTexCoord2f(tx0, ty1);
    glVertex2i(ix, fy);
    glTexCoord2f(tx0, ty1);
    glVertex2i(ix, fy);
    glTexCoord2f(tx1, ty0);
    glVertex2i(fx, iy);
    glTexCoord2f(tx1, ty1);
    glVertex2i(fx, fy);
    glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
}

void Display_Avancement(float avanct, int step, int nsteps)
{
    float avancement = (step + avanct) / nsteps;
    Draw_Fill_Rectangle_Text(glfwframe, 0, 0, (int)(ScrW * avancement), ScrH, TxcRom, 0, avancement, 0, 2.15f);
    gl33_SwapBuffers(glfwframe, false);
}

void Draw_Raw_Digit(UINT8 digit, UINT x, UINT y, UINT8* pbuf, UINT width, UINT height)
{
    // Draw a digit in a RGBA memory buffer pbuf at (x,y) dimension of the buffer (width,height)
    UINT8* pdig = &Raw_Digit_Def[digit * RAW_DIGIT_W];
    UINT8* pdig2;
    const UINT dwid = 10 * RAW_DIGIT_W;
    UINT mx = x + RAW_DIGIT_W;
    if (mx > width) mx = width;
    UINT my = y + RAW_DIGIT_H;
    if (my > height) my = height;
    for (UINT tj = y; tj < my; tj++)
    {
        pdig2 = pdig;
        for (UINT ti = x; ti < mx; ti++)
        {
            float lval = (float)(255-(*pdig)) / 255.0f;
            UINT val = (UINT)((1.0f - lval) * pbuf[ti * 4 + tj * 4 * width] + lval * draw_color[0]);
            if (val > 255) pbuf[ti * 4 + tj * 4 * width] = 255; else pbuf[ti * 4 + tj * 4 * width] = val;
            val = (UINT)((1.0f - lval) * pbuf[ti * 4 + tj * 4 * width + 1] + lval * draw_color[1]);
            if (val > 255) pbuf[ti * 4 + tj * 4 * width + 1] = 255; else pbuf[ti * 4 + tj * 4 * width + 1] = val;
            val = (UINT)((1.0f - lval) * pbuf[ti * 4 + tj * 4 * width + 2] + lval * draw_color[2]);
            if (val > 255) pbuf[ti * 4 + tj * 4 * width + 2] = 255; else pbuf[ti * 4 + tj * 4 * width + 2] = val;
            pbuf[ti * 4 + tj * 4 * width + 3] = 255;
            pdig++;
        }
        pdig = pdig2 + dwid;
    }
}

void Draw_Raw_Number(UINT number, UINT x, UINT y, UINT8* pbuf, UINT width, UINT height)
{
    // Draw a number to opengl from a texture giving the 10 digits
    UINT div = 1000000000;
    bool started = false;
    UINT num = number;
    UINT tx = x;
    while (div > 0)
    {
        if (started || (num / div > 0) || (div == 1))
        {
            started = true;
            UINT8 digit = (UINT8)(num / div);
            Draw_Raw_Digit(digit, tx, y, pbuf, width, height);
            num = num - div * digit;
            tx += RAW_DIGIT_W;
        }
        div = div / 10;
    }
}

void Draw_Digit(UINT8 digit, UINT x, UINT y, float zoom)
{
    // Draw a digit to opengl with a texture (to use with Draw_Number)
    glColor4ubv(draw_color);
    glTexCoord2f(digit / 10.0f, 0);
    glVertex2i(x, y);
    glTexCoord2f((digit + 1) / 10.0f, 0);
    glVertex2i(x + (int)(zoom * DIGIT_TEXTURE_W), y);
    glTexCoord2f((digit + 1) / 10.0f, 1);
    glVertex2i(x + (int)(zoom * DIGIT_TEXTURE_W), y + (int)(zoom * DIGIT_TEXTURE_H));
    glTexCoord2f(digit / 10.0f, 1);
    glVertex2i(x, y + (int)(zoom * DIGIT_TEXTURE_H));
    glTexCoord2f(digit / 10.0f, 0);
    glVertex2i(x, y);
    glTexCoord2f((digit + 1) / 10.0f, 1);
    glVertex2i(x + (int)(zoom * DIGIT_TEXTURE_W), y + (int)(zoom * DIGIT_TEXTURE_H));
}

void ConvertSurfaceToFrame(UINT8* surface, bool isDel)
{
    // convert a mask in the surface into a drawing on the selected frames
    for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
    {
        if (surface[ti] == TRUE)
        {
            UINT8 col = acEditColors[noColSel];
            for (UINT tj = 0; tj < nSelFrames; tj++)
            {
                if (isDel) col = MycRP.oFrames[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti];
                else if (MycRP.DrawColMode == 1) col = acEditColors[MycRP.oFrames[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti]];
                MycRom.cFrames[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti] = col;
            }
        }
    }
}

void ConvertSurfaceToDetection(UINT8* surface)
{
    UINT16 xmin = 0xffff, xmax = 0, ymin = 0xffff, ymax = 0;
    for (UINT tj = 0; tj < MAX_SPRITE_SIZE; tj++)
    {
        for (UINT ti = 0; ti < MAX_SPRITE_SIZE; ti++)
        {
            if (surface[ti+tj*MAX_SPRITE_SIZE] == TRUE)
            {
                if (xmin > ti) xmin = ti;
                if (ymin > tj) ymin = tj;
                if (xmax < ti) xmax = ti;
                if (ymax < tj) ymax = tj;
            }
        }
    }
    xmax = xmax - xmin + 1;
    ymax = ymax - ymin + 1;
    MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS + acDetSprite * 4] = xmin;
    MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS + acDetSprite * 4 + 1] = ymin;
    MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS + acDetSprite * 4 + 2] = xmax;
    MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS + acDetSprite * 4 + 3] = ymax;
}

void ConvertSurfaceToSprite(UINT8* surface)
{
    // convert a mask in the surface into a drawing on the selected frames
    for (UINT ti = 0; ti < MAX_SPRITE_SIZE*MAX_SPRITE_SIZE; ti++)
    {
        UINT8 col = MycRP.Sprite_Edit_Colors[acSprite * 16 + noSprSel];
        if ((surface[ti] == TRUE) && !(MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + ti] & 0x8000))
        {
            //if (MycRP.DrawColMode == 1) col = acEditColors[MycRP.oFrames[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti]];
            MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + ti] &= 0xff00;
            MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + ti] |= (UINT16)col;
        }
    }
}

void ConvertCopyToGradient(int xd, int yd, int xf, int yf)
{
    float pscal[256 * 64]; // array for calculating scalar product
    /*float psmin = 1000000;
    float psmax = -1000000;*/
    // (vx,vy) unitary direction vertex of the gradient
    float vx = (float)xf - (float)xd;
    float vy = (float)yf - (float)yd;
    float vnorm = (float)sqrt(vx * vx + vy * vy);
    vx /= vnorm;
    vy /= vnorm;
    for (int tj = 0; tj < (int)MycRom.fHeight; tj++)
    {
        for (int ti = 0; ti < (int)MycRom.fWidth; ti++)
        {
            pscal[tj * MycRom.fWidth + ti] = (float)(ti - xd) / vnorm * vx + (float)(tj - yd) / vnorm * vy;
        }
    }
    float range = (float)(max(Draw_Grad_Fin, Draw_Grad_Ini) - min(Draw_Grad_Fin, Draw_Grad_Ini) + 1);
    for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
    {
        if (Copy_Mask[ti] > 0)
        {
            for (UINT tj = 0; tj < nSelFrames; tj++)
            {
                UINT8 fcol;
                if (pscal[ti] <= 0) fcol = min(Draw_Grad_Fin, Draw_Grad_Ini);
                else if (pscal[ti] >= 1) fcol = max(Draw_Grad_Fin, Draw_Grad_Ini);
                else if ((pscal[ti] >= 0) && (pscal[ti] <= 1)) fcol = min(Draw_Grad_Ini, Draw_Grad_Fin) + (UINT8)((pscal[ti] * range));
                MycRom.cFrames[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti] = fcol;
            }
        }
    }
}

float distance(int cx,int cy,int x, int y)
{
    float vx = (float)cx - (float)x;
    float vy = (float)cy - (float)y;
    return (float)sqrt(vx * vx + vy * vy);
}

void ConvertCopyToRadialGradient(int xd, int yd, int xf, int yf)
{
    float vnorm = distance(xf, yf, xd, yd);
    float range = (float)(max(Draw_Grad_Fin, Draw_Grad_Ini) - min(Draw_Grad_Fin, Draw_Grad_Ini) + 1);
    for (int tj = 0; tj < (int)MycRom.fHeight; tj++)
    {
        for (int ti = 0; ti < (int)MycRom.fWidth; ti++)
        {
            if (Copy_Mask[tj * MycRom.fWidth + ti] > 0)
            {
                float rayon = distance(xd, yd, ti, tj);

                if (rayon >= vnorm)
                {
                    for (unsigned int tk = 0; tk < nSelFrames; tk++) MycRom.cFrames[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight + tj * MycRom.fWidth + ti] =
                        max(Draw_Grad_Fin, Draw_Grad_Ini);
                }
                else
                {
                    for (unsigned int tk = 0; tk < nSelFrames; tk++) MycRom.cFrames[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight + tj * MycRom.fWidth + ti] =
                        min(Draw_Grad_Ini, Draw_Grad_Fin) + (int)((rayon / vnorm) * range);
                }
            }
        }
    }
}

void EmptyExtraSurface(void)
{
    memset(Draw_Extra_Surface, 0, 256 * 64);
}

void EmptyExtraSurface2(void)
{
    memset(Draw_Extra_Surface2, 0, MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
}

void putpixel(int x, int y, UINT8* surface, UINT8 color, bool coloronly, byte* frame)
{
    // set a pixel in a monochrome memory surface or frame
    if ((x < 0) || (x >= (int)MycRom.fWidth) || (y < 0) || (y >= (int)MycRom.fHeight)) return;
    if (coloronly) // do we just mask the non-0 pixels
    {
        if (frame[y * MycRom.fWidth + x] == 0) return;
    }
    surface[y * MycRom.fWidth + x] = color;
}

void putpixel2(int x, int y, UINT8* surface, UINT8 color)
{
    // set a pixel in a monochrome memory surface or frame for sprites
    if ((x < 0) || (x >= (int)MAX_SPRITE_SIZE) || (y < 0) || (y >= (int)MAX_SPRITE_SIZE)) return;
    surface[y * MAX_SPRITE_SIZE + x] = color;
}

void drawline(int x, int y, int x2, int y2, UINT8* surface, UINT8 color, bool coloronly, byte* frame)
{
    int w = x2 - x;
    int h = y2 - y;
    int dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0;
    if (w < 0) dx1 = -1; else if (w > 0) dx1 = 1;
    if (h < 0) dy1 = -1; else if (h > 0) dy1 = 1;
    if (w < 0) dx2 = -1; else if (w > 0) dx2 = 1;

    int longest = abs(w);
    int shortest = abs(h);

    if (!(longest > shortest))
    {
        longest = abs(h);
        shortest = abs(w);
        if (h < 0) dy2 = -1;
        else if (h > 0) dy2 = 1;
        dx2 = 0;
    }
    int numerator = longest >> 1;
    for (int i = 0; i <= longest; i++)
    {
        putpixel(x, y, surface, color, coloronly, frame);
        numerator += shortest;
        if (!(numerator < longest))
        {
            numerator -= longest;
            x += dx1;
            y += dy1;
        }
        else {
            x += dx2;
            y += dy2;
        }
    }
}

void drawline2(int x, int y, int x2, int y2, UINT8* surface, UINT8 color)
{
    int w = x2 - x;
    int h = y2 - y;
    int dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0;
    if (w < 0) dx1 = -1; else if (w > 0) dx1 = 1;
    if (h < 0) dy1 = -1; else if (h > 0) dy1 = 1;
    if (w < 0) dx2 = -1; else if (w > 0) dx2 = 1;

    int longest = abs(w);
    int shortest = abs(h);

    if (!(longest > shortest))
    {
        longest = abs(h);
        shortest = abs(w);
        if (h < 0) dy2 = -1;
        else if (h > 0) dy2 = 1;
        dx2 = 0;
    }
    int numerator = longest >> 1;
    for (int i = 0; i <= longest; i++)
    {
        putpixel2(x, y, surface, color);
        numerator += shortest;
        if (!(numerator < longest))
        {
            numerator -= longest;
            x += dx1;
            y += dy1;
        }
        else {
            x += dx2;
            y += dy2;
        }
    }
}

void drawAllOctantsF(int xc, int yc, int xp, int yp, UINT8* surface, UINT8 color, bool coloronly, byte* frame)
{
    for (int x = 0; x <= xp; x++)
    {
        for (int y = 0; y <= yp; y++)
        {
            putpixel(xc + x, yc + y, surface, color, coloronly, frame);
            putpixel(xc - x, yc + y, surface, color, coloronly, frame);
            putpixel(xc + x, yc - y, surface, color, coloronly, frame);
            putpixel(xc - x, yc - y, surface, color, coloronly, frame);
            putpixel(xc + y, yc + x, surface, color, coloronly, frame);
            putpixel(xc - y, yc + x, surface, color, coloronly, frame);
            putpixel(xc + y, yc - x, surface, color, coloronly, frame);
            putpixel(xc - y, yc - x, surface, color, coloronly, frame);
        }
    }
}

void drawAllOctants(int xc, int yc, int x, int y, UINT8* surface, UINT8 color, bool coloronly, byte* frame)
{
    putpixel(xc + x, yc + y, surface, color, coloronly, frame);
    putpixel(xc - x, yc + y, surface, color, coloronly, frame);
    putpixel(xc + x, yc - y, surface, color, coloronly, frame);
    putpixel(xc - x, yc - y, surface, color, coloronly, frame);
    putpixel(xc + y, yc + x, surface, color, coloronly, frame);
    putpixel(xc - y, yc + x, surface, color, coloronly, frame);
    putpixel(xc + y, yc - x, surface, color, coloronly, frame);
    putpixel(xc - y, yc - x, surface, color, coloronly, frame);
}


void drawAllOctantsF2(int xc, int yc, int xp, int yp, UINT8* surface, UINT8 color)
{
    for (int x = 0; x <= xp; x++)
    {
        for (int y = 0; y <= yp; y++)
        {
            putpixel2(xc + x, yc + y, surface, color);
            putpixel2(xc - x, yc + y, surface, color);
            putpixel2(xc + x, yc - y, surface, color);
            putpixel2(xc - x, yc - y, surface, color);
            putpixel2(xc + y, yc + x, surface, color);
            putpixel2(xc - y, yc + x, surface, color);
            putpixel2(xc + y, yc - x, surface, color);
            putpixel2(xc - y, yc - x, surface, color);
        }
    }
}

void drawAllOctants2(int xc, int yc, int x, int y, UINT8* surface, UINT8 color)
{
    putpixel2(xc + x, yc + y, surface, color);
    putpixel2(xc - x, yc + y, surface, color);
    putpixel2(xc + x, yc - y, surface, color);
    putpixel2(xc - x, yc - y, surface, color);
    putpixel2(xc + y, yc + x, surface, color);
    putpixel2(xc - y, yc + x, surface, color);
    putpixel2(xc + y, yc - x, surface, color);
    putpixel2(xc - y, yc - x, surface, color);
}

void drawcircle(int xc, int yc, int r, UINT8* surface, UINT8 color, BOOL filled, bool coloronly, byte* frame)
{
    // draw a circle with the bresenham algorithm in a monochrome memory surface (same size as the frames) or frame
    int x = 0, y = r;
    int d = 3 - 2 * r;
    if (!filled) drawAllOctants(xc, yc, x, y, surface, color, coloronly, frame); else drawAllOctantsF(xc, yc, x, y, surface, color, coloronly, frame);
    while (y >= x)
    {
        x++;
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
            d = d + 4 * x + 6;
        if (!filled) drawAllOctants(xc, yc, x, y, surface, color, coloronly, frame); else drawAllOctantsF(xc, yc, x, y, surface, color, coloronly, frame);
    }
}

void drawcircle2(int xc, int yc, int r, UINT8* surface, UINT8 color, BOOL filled)
{
    // draw a circle with the bresenham algorithm in a monochrome memory surface (same size as the frames) or frame
    int x = 0, y = r;
    int d = 3 - 2 * r;
    if (!filled) drawAllOctants2(xc, yc, x, y, surface, color); else drawAllOctantsF2(xc, yc, x, y, surface, color);
    while (y >= x)
    {
        x++;
        if (d > 0)
        {
            y--;
            d = d + 4 * (x - y) + 10;
        }
        else
            d = d + 4 * x + 6;
        if (!filled) drawAllOctants2(xc, yc, x, y, surface, color); else drawAllOctantsF2(xc, yc, x, y, surface, color);
    }
}

void drawrectangle(int x, int y, int x2, int y2, UINT8* surface, UINT8 color, BOOL isfilled, bool coloronly, byte* frame)
{
    for (int tj = min(x, x2); tj <= max(x, x2); tj++)
    {
        for (int ti = min(y, y2); ti <= max(y, y2); ti++)
        {
            if ((tj != x) && (tj != x2) && (ti != y) && (ti != y2) && (!isfilled)) continue;
            if (coloronly) // do we just mask the non-0 pixels
            {
                if (frame[ti * MycRom.fWidth + tj] == 0) continue;
            }
            surface[ti * MycRom.fWidth + tj] = color;
        }
    }
}

void drawrectangle2(int x, int y, int x2, int y2, UINT8* surface, UINT8 color, BOOL isfilled)
{
    // for sprites
    for (int tj = min(x, x2); tj <= max(x, x2); tj++)
    {
        for (int ti = min(y, y2); ti <= max(y, y2); ti++)
        {
            if ((tj != x) && (tj != x2) && (ti != y) && (ti != y2) && (!isfilled)) continue;
            surface[ti * MAX_SPRITE_SIZE + tj] = color;
        }
    }
}

void floodfill(int x, int y, UINT8* surface, UINT8 scolor, UINT8 sdyn, UINT8 dcolor)
{
    if ((x < 0) || (x >= (int)MycRom.fWidth) || (y < 0) || (y >= (int)MycRom.fHeight)) return;
    if (MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight + y * MycRom.fWidth + x] != sdyn) return;
    if ((sdyn == 255) && (MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight + y * MycRom.fWidth + x] != scolor)) return;
    if (surface[y * MycRom.fWidth + x] == dcolor) return;
    surface[y * MycRom.fWidth + x] = dcolor;
    floodfill(x + 1, y, surface, scolor, sdyn, dcolor);
    floodfill(x - 1, y, surface, scolor, sdyn, dcolor);
    floodfill(x, y + 1, surface, scolor, sdyn, dcolor);
    floodfill(x, y - 1, surface, scolor, sdyn, dcolor);
}

void floodfill2(int x, int y, UINT8* surface, UINT8 scolor, UINT8 dcolor)
{
    if ((x < 0) || (x >= (int)MycRom.fWidth) || (y < 0) || (y >= (int)MycRom.fHeight)) return;
    if (MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + y * MycRom.fWidth + x] != scolor) return;
    if (surface[y * MycRom.fWidth + x] == dcolor) return;
    surface[y * MycRom.fWidth + x] = dcolor;
    floodfill2(x + 1, y, surface, scolor, dcolor);
    floodfill2(x - 1, y, surface, scolor, dcolor);
    floodfill2(x, y + 1, surface, scolor, dcolor);
    floodfill2(x, y - 1, surface, scolor, dcolor);
}

void floodfill3(int x, int y, UINT8* surface, UINT8 scolor, UINT8 dcolor)
{
    if ((x < 0) || (x >= (int)MAX_SPRITE_SIZE) || (y < 0) || (y >= (int)MAX_SPRITE_SIZE)) return;
    if ((MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + y * MAX_SPRITE_SIZE + x] & 0xff00) == 0xff00) return;
    if ((UINT8)(MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + y * MAX_SPRITE_SIZE + x] & 0xff) != scolor) return;
    if (surface[y * MAX_SPRITE_SIZE + x] == dcolor) return;
    surface[y * MAX_SPRITE_SIZE + x] = dcolor;
    floodfill3(x + 1, y, surface, scolor, dcolor);
    floodfill3(x - 1, y, surface, scolor, dcolor);
    floodfill3(x, y + 1, surface, scolor, dcolor);
    floodfill3(x, y - 1, surface, scolor, dcolor);
}

void drawfill(int x, int y, UINT8* surface, UINT8 color)
{
    // first we get the color value at the clicked position: version for colorized frame
    // in this version we must also take into account the dynamic mask value
    UINT8 searchcol = MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight + y * MycRom.fWidth + x];
    UINT8 searchdyn = MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight + y * MycRom.fWidth + x];
    floodfill(x, y, surface, searchcol, searchdyn, color);
}

void drawfill2(int x, int y, UINT8* surface, UINT8 color)
{
    // first we get the color value at the clicked position: version for original frame 
    UINT8 searchcol = MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + y * MycRom.fWidth + x];
    floodfill2(x, y, surface, searchcol, color);
}

void drawfill3(int x, int y, UINT8* surface, UINT8 color)
{
    // first we get the color value at the clicked position: version for original frame 
    if ((MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + y * MAX_SPRITE_SIZE + x] & 0xff00) == 0xff00) return;
    UINT8 searchcol = (UINT)(MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + y * MAX_SPRITE_SIZE + x] & 0xff);
    floodfill3(x, y, surface, searchcol, color);
}

void Draw_Line(float x0, float y0, float x1, float y1)
{
    glVertex2i((int)x0, (int)y0);
    glVertex2i((int)x1, (int)y1);
}

void Draw_Over_From_Surface(UINT8* Surface, UINT8 val, float zoom, bool checkfalse, bool invert)
{
    // using the Surface, we draw the contour of pixels with a value of val and check the other pixels according checkfalse and invert
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
    glColor4ubv(draw_color);

    for (UINT tj = 0; tj < MycRom.fHeight; tj++)
    {
        for (UINT ti = 0; ti < MycRom.fWidth; ti++)
        {
            UINT tk = tj * MycRom.fWidth + ti;
            if (((Surface[tk] != val) && (!invert)) || ((Surface[tk] == val) && (invert)))
            {
                if (tj == 0) Draw_Line(ti * zoom, 0, (ti + 1) * zoom, 0);
                else if (((Surface[tk - MycRom.fWidth] == val) && (!invert)) || ((Surface[tk - MycRom.fWidth] != val) && (invert))) Draw_Line(ti * zoom, tj * zoom, (ti + 1) * zoom, tj * zoom);

                if (tj == MycRom.fHeight - 1) Draw_Line(ti * zoom, MycRom.fHeight * zoom, (ti + 1) * zoom, MycRom.fHeight * zoom - 1);
                else if (((Surface[tk + MycRom.fWidth] == val) && (!invert)) || ((Surface[tk + MycRom.fWidth] != val) && (invert))) Draw_Line(ti * zoom, (tj + 1) * zoom, (ti + 1) * zoom, (tj + 1) * zoom);

                if (ti == 0) Draw_Line(0, tj * zoom, 0, (tj + 1) * zoom - 1);
                else if (((Surface[tk - 1] == val) && (!invert)) || ((Surface[tk - 1] != val) && (invert))) Draw_Line(ti * zoom, tj * zoom, ti * zoom, (tj + 1) * zoom);

                if (ti == MycRom.fWidth - 1) Draw_Line(MycRom.fWidth * zoom, tj * zoom, MycRom.fWidth * zoom, (tj + 1) * zoom);
                else if (((Surface[tk + 1] == val) && (!invert)) || ((Surface[tk + 1] != val) && (invert))) Draw_Line((ti + 1) * zoom, tj * zoom, (ti + 1) * zoom, (tj + 1) * zoom);
            }
            else if (checkfalse)
            {
                Draw_Line(ti * zoom, tj * zoom, (ti + 1) * zoom, (tj + 1) * zoom);
                Draw_Line(ti * zoom, (tj + 1) * zoom, (ti + 1) * zoom, tj * zoom);
            }
        }
    }
    glEnd();
}

void Draw_Over_From_Surface2(UINT8* Surface, UINT8 val, float zoom, bool checkfalse, bool invert,bool original)
{
    // using the Surface, we draw the contour of pixels with a value of val and check the other pixels according checkfalse and invert
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
    glColor4ubv(draw_color);
    float xoffset = MAX_SPRITE_SIZE * zoom + SPRITE_INTERVAL;
    if (original) xoffset = 0;
    for (UINT tj = 0; tj < MAX_SPRITE_SIZE; tj++)
    {
        for (UINT ti = 0; ti < MAX_SPRITE_SIZE; ti++)
        {
            UINT tk = tj * MAX_SPRITE_SIZE + ti;
            if (((Surface[tk] != val) && (!invert)) || ((Surface[tk] == val) && (invert)))
            {
                if (tj == 0) Draw_Line(xoffset + ti * zoom, 0, xoffset + (ti + 1) * zoom, 0);
                else if (((Surface[tk - MAX_SPRITE_SIZE] == val) && (!invert)) || ((Surface[tk - MAX_SPRITE_SIZE] != val) && (invert))) Draw_Line(xoffset + ti * zoom, tj * zoom, xoffset + (ti + 1) * zoom, tj * zoom);

                if (tj == MAX_SPRITE_SIZE - 1) Draw_Line(xoffset + ti * zoom, MAX_SPRITE_SIZE * zoom, xoffset + (ti + 1) * zoom, MAX_SPRITE_SIZE * zoom - 1);
                else if (((Surface[tk + MAX_SPRITE_SIZE] == val) && (!invert)) || ((Surface[tk + MAX_SPRITE_SIZE] != val) && (invert))) Draw_Line(xoffset + ti * zoom, (tj + 1) * zoom, xoffset + (ti + 1) * zoom, (tj + 1) * zoom);

                if (ti == 0) Draw_Line(xoffset , tj * zoom, xoffset, (tj + 1) * zoom - 1);
                else if (((Surface[tk - 1] == val) && (!invert)) || ((Surface[tk - 1] != val) && (invert))) Draw_Line(xoffset + ti * zoom, tj * zoom, xoffset + ti * zoom, (tj + 1) * zoom);

                if (ti == MAX_SPRITE_SIZE - 1) Draw_Line(xoffset + MAX_SPRITE_SIZE * zoom, tj * zoom, xoffset + MAX_SPRITE_SIZE * zoom, (tj + 1) * zoom);
                else if (((Surface[tk + 1] == val) && (!invert)) || ((Surface[tk + 1] != val) && (invert))) Draw_Line(xoffset + (ti + 1) * zoom, tj * zoom, xoffset + (ti + 1) * zoom, (tj + 1) * zoom);
            }
            else if (checkfalse)
            {
                Draw_Line(xoffset + ti * zoom, tj * zoom, xoffset + (ti + 1) * zoom, (tj + 1) * zoom);
                Draw_Line(xoffset + ti * zoom, (tj + 1) * zoom, xoffset + (ti + 1) * zoom, tj * zoom);
            }
        }
    }
    glEnd();
}

void Draw_Paste_Over(GLFWwindow* glfwin,UINT x,UINT y,UINT zoom)
{
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, TxCircle);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    for (int tj = 0; tj < (int)Paste_Height; tj++)
    {
        for (int ti = 0; ti < (int)Paste_Width; ti++)
        {
            int i = ti;
            int j = tj;
            if (Paste_Mirror & 1) i = Paste_Width - 1 - ti;
            if (Paste_Mirror & 2) j = Paste_Height - 1 - tj;
            if (Paste_Mask[i + j * Paste_Width] == 0) continue;
            if ((ti + paste_offsetx < 0) || (ti + paste_offsetx >= (int)MycRom.fWidth) || (tj + paste_offsety < 0) || (tj + paste_offsety >= (int)MycRom.fHeight)) continue;
            SetRenderDrawColorv(&MycRom.cPal[Copy_From_Frame * 3 * MycRom.ncColors + Paste_Col[i + j * Paste_Width] * 3], mselcol);
            RenderDrawPoint(glfwin, x + (ti + paste_offsetx)* zoom, y + (tj + paste_offsety) * zoom, zoom);
        }
    }
    glEnd();
    /*SetRenderDrawColor(0, mselcol, mselcol, 255);
    Draw_Over_From_Surface(Paste_Mask, 0, (float)zoom, false, true);*/

}

void Draw_Number(UINT number, UINT x, UINT y, float zoom)
{
    // Draw a number to opengl from a texture giving the 10 digits
    UINT div = 1000000000;
    bool started = false;
    UINT num=number;
    UINT tx = x;
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, TxChiffres);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    while (div > 0)
    {
        if (started || (num / div > 0) || (div == 1))
        {
            started = true;
            UINT8 digit = (UINT8)(num / div);
            Draw_Digit(digit, tx, y, zoom);
            num = num - div * digit;
            tx += (UINT)(zoom * DIGIT_TEXTURE_W);
        }
        div = div / 10;
    }
    glEnd();
}

void SetViewport(GLFWwindow* glfwin)
{
    // Set the OpenGL viewport in 2D according the client area of the child window
    int Resx, Resy;
    glfwMakeContextCurrent(glfwin);
    glfwGetFramebufferSize(glfwin, &Resx, &Resy);
    glViewport(0, 0, Resx, Resy);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, Resx, Resy, 0, -2, 2);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Calc_Resize_Frame(void)
{
    // Calculate the global variables depending on the main window dimension
    glfwMakeContextCurrent(glfwframe);
    RECT winrect;
    GetClientRect(hWnd, &winrect);
    ScrW = winrect.right;
    ScrH = winrect.bottom;
    NFrameToDraw = (int)((float)(ScrW - FRAME_STRIP_W_MARGIN) / (float)(256 + FRAME_STRIP_W_MARGIN)); // Calculate the number of frames to display in the strip
    FS_LMargin = (ScrW - (NFrameToDraw * (FRAME_STRIP_W_MARGIN + 256) + FRAME_STRIP_W_MARGIN)) / 2; // calculate the left and right margin in the strip
    if (MycRom.name[0])
    {
        if (MycRom.fWidth == 192)
        {
            NFrameToDraw = (int)((float)(ScrW - FRAME_STRIP_W_MARGIN) / (float)(192 + FRAME_STRIP_W_MARGIN)); // Calculate the number of frames to display in the strip
            FS_LMargin = (ScrW - (NFrameToDraw * (FRAME_STRIP_W_MARGIN + 192) + FRAME_STRIP_W_MARGIN)) / 2; // calculate the left and right margin in the strip
        }
    }
    int thei = winrect.bottom - (TOOLBAR_HEIGHT + 20) - FRAME_STRIP_HEIGHT - statusBarHeight - 20;
    int twid = winrect.right;
    int mul = 4;
    if (MycRom.name[0])
    {
        if (MycRom.fWidth == 192) mul = 3;
    }
    if (((float)twid / (float)thei) > mul) twid = thei * mul; else
    {
        thei = twid / mul;
        twid = thei * mul; // in case the previous division doesn't exactly give winrect.right == winrect.bottom * 4
    }
    if (MycRom.name[0])
    {
        frame_zoom = (int)((float)thei / (float)MycRom.fHeight); // We adjust to an int value of zoom
        twid = frame_zoom * MycRom.fWidth;
        thei = frame_zoom * MycRom.fHeight;
    }
    else
    {
        frame_zoom = (int)((float)thei / (float)32); // default height value 32 for the roms
        twid = frame_zoom * 128; // default width value 128 for the roms
        thei = frame_zoom * 32;
    }
    glfwSetWindowSize(glfwframe, twid, thei);
    offset_frame_x = (winrect.right - twid) / 2;
    offset_frame_y = TOOLBAR_HEIGHT + 10 + (winrect.bottom - (TOOLBAR_HEIGHT + 20) - thei - FRAME_STRIP_HEIGHT) / 2;
    glfwSetWindowPos(glfwframe, offset_frame_x, offset_frame_y);
    SetViewport(glfwframe);
    glfwSetWindowSize(glfwframestrip, ScrW, FRAME_STRIP_HEIGHT);
    glfwSetWindowPos(glfwframestrip, 0, ScrH - FRAME_STRIP_HEIGHT - statusBarHeight);
    SetViewport(glfwframestrip);
}

void Calc_Resize_Sprite(void)
{
    // Calculate the global variables depending on the main window dimension
    glfwMakeContextCurrent(glfwsprites);
    RECT winrect;
    GetClientRect(hSprites, &winrect);
    ScrW2 = winrect.right;
    ScrH2 = winrect.bottom;
    NSpriteToDraw = (int)((float)(ScrW2 - FRAME_STRIP_W_MARGIN) / (float)(2* MAX_SPRITE_SIZE + FRAME_STRIP_W_MARGIN)); // Calculate the number of frames to display in the strip
    SS_LMargin = (ScrW2 - (NSpriteToDraw * (FRAME_STRIP_W_MARGIN + 2* MAX_SPRITE_SIZE) + FRAME_STRIP_W_MARGIN)) / 2; // calculate the left and right margin in the strip
    int thei = winrect.bottom - (TOOLBAR_HEIGHT + 20) - FRAME_STRIP_HEIGHT2 - statusBarHeight - 20;
    int twid = winrect.right;
    float mul = 2 + SPRITE_INTERVAL / (float)MAX_SPRITE_SIZE;
    if (((float)twid / (float)thei) > mul) twid = (int)(thei * mul); else
    {
        thei = (int)(twid / mul);
        twid = (int)(thei * mul); // in case the previous division doesn't exactly give winrect.right == winrect.bottom * 4
    }
    sprite_zoom = (int)((float)thei / (float)MAX_SPRITE_SIZE); // We adjust to an int value of zoom
    twid = sprite_zoom * MAX_SPRITE_SIZE * 2 + SPRITE_INTERVAL;
    thei = sprite_zoom * MAX_SPRITE_SIZE;
    glfwSetWindowSize(glfwsprites, twid, thei);
    offset_sprite_x = (winrect.right - twid) / 2;
    offset_sprite_y = TOOLBAR_HEIGHT + 10 + (winrect.bottom - (TOOLBAR_HEIGHT + 20) - thei - FRAME_STRIP_HEIGHT2) / 2;
    glfwSetWindowPos(glfwsprites, offset_sprite_x, offset_sprite_y);
    SetViewport(glfwsprites);
    glfwSetWindowSize(glfwspritestrip, ScrW2, FRAME_STRIP_HEIGHT2);
    glfwSetWindowPos(glfwspritestrip, 0, ScrH2 - FRAME_STRIP_HEIGHT2 - statusBarHeight);
    SetViewport(glfwspritestrip);
}



unsigned char RGBMask[3] = { 255,255,255 };

void MaskCommonPoints(UINT8* surface)// , bool checkblack)
{
    // check points with same color in the selected frames
    memset(surface, 1, MycRom.fWidth * MycRom.fHeight); // initially, we consider all the points as identical
    if (nSelFrames < 2) return;
    for (UINT tj = 0; tj < MycRom.fWidth * MycRom.fHeight; tj++)
    {
        for (UINT ti = 1; ti < nSelFrames; ti++)
        {
            // we just compare all the selected frames to the first one and as soon as the pixel is different we 0 it
            if (MycRP.oFrames[SelFrames[0] * MycRom.fWidth * MycRom.fHeight + tj] != MycRP.oFrames[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight + tj])
            {
                surface[tj] = 0;
                break;
            }
        }
    }
}

void RenderDrawPointClip(GLFWwindow* glfwin, unsigned int x, unsigned int y, unsigned int xmax, unsigned int ymax, unsigned int zoom)
{
    // square out of the clipping zone
    if ((x > xmax) || (y > ymax)) return;
    // square entirely in the clipping zone
    if ((x + zoom - 1 <= xmax) && (y + zoom - 1 <= ymax))
    {
        glColor4ubv(draw_color);
        glTexCoord2f(0, 0);
        glVertex2i(x, y);
        glTexCoord2f(1, 0);
        glVertex2i(x + zoom - 1, y);
        glTexCoord2f(1, 1);
        glVertex2i(x + zoom - 1, y + zoom - 1);
        glTexCoord2f(0, 1);
        glVertex2i(x, y + zoom - 1);
        glTexCoord2f(0, 0);
        glVertex2i(x, y);
        glTexCoord2f(1, 1);
        glVertex2i(x + zoom - 1, y + zoom - 1);
        return;
    }
    // square partially out of the clipping zone
    unsigned int tx = x + zoom - 1;
    unsigned int ty = y + zoom - 1;
    float ttx = 1, tty = 1;
    if (tx > xmax)
    {
        tx = xmax;
        ttx = (float)(xmax - x) / (float)zoom;
    }
    if (ty > ymax)
    {
        ty = ymax;
        tty = (float)(ymax - y) / (float)zoom;
    }
    glColor4ubv(draw_color);
    glTexCoord2f(0, 0);
    glVertex2i(x, y);
    glTexCoord2f(ttx, 0);
    glVertex2i(tx, y);
    glTexCoord2f(ttx, tty);
    glVertex2i(tx, ty);
    glTexCoord2f(0, tty);
    glVertex2i(x, tx);
    glTexCoord2f(0, 0);
    glVertex2i(x, y);
    glTexCoord2f(ttx, tty);
    glVertex2i(tx, ty);
}

void RenderDrawPoint(GLFWwindow* glfwin, unsigned int x, unsigned int y, unsigned int zoom)
{
    glColor4ubv(draw_color);
    glTexCoord2f(0, 0);
    glVertex2i(x, y);
    glTexCoord2f(1, 0);
    glVertex2i(x + zoom - 1, y);
    glTexCoord2f(1, 1);
    glVertex2i(x + zoom - 1, y + zoom - 1);
    glTexCoord2f(0, 1);
    glVertex2i(x, y + zoom - 1);
    glTexCoord2f(0, 0);
    glVertex2i(x, y);
    glTexCoord2f(1, 1);
    glVertex2i(x + zoom - 1, y + zoom - 1);
}

void Draw_Frame(GLFWwindow* glfwin, unsigned int zoom, unsigned int nofr, unsigned int x, unsigned int y, unsigned int clipw, unsigned int cliph, bool showRects, bool original)
{
    // display the frame nofr in the opengl window *glfwin at pos (x,y) with dimensions (clipw,cliph)
    // showRects tells if the rectangle contours must be shown
    // original is true if we display the original frame, false if we display the colorized frame

    if (nofr >= MycRom.nFrames)
    {
        cprintf("Unknown frame requested in Draw_Frame");
        return;
    }
    glfwMakeContextCurrent(glfwin);
    glEnable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, TxCircle);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    UINT8* pFrame = &MycRom.cFrames[nofr * MycRom.fWidth * MycRom.fHeight];
    UINT8* pFramePal = &MycRom.cPal[nofr * 3 * MycRom.ncColors];
    if (original)
    {
        pFrame = &MycRP.oFrames[nofr * MycRom.fWidth * MycRom.fHeight];
        pFramePal = originalcolors;
    }
    // color rotation
    UINT8 tColRot[64];
    DWORD actime = timeGetTime();
    for (UINT ti = 0; ti < 64; ti++) tColRot[ti] = ti;
    for (UINT ti = 0; ti < MAX_COLOR_ROTATION; ti++)
    {
        if (MyColRot.firstcol[ti]!= 255)
        {
            if (actime >= MyColRot.lastTime[ti] + MyColRot.timespan[ti])
            {
                MyColRot.lastTime[ti] = actime;
                MyColRot.acfirst[ti]++;
                if (MyColRot.acfirst[ti] == MyColRot.ncol[ti]) MyColRot.acfirst[ti] = 0;
            }
            for (UINT8 tj = 0; tj < MyColRot.ncol[ti]; tj++)
            {
                tColRot[tj + MyColRot.firstcol[ti]] = tj + MyColRot.firstcol[ti] + MyColRot.acfirst[ti];
                if (tColRot[tj + MyColRot.firstcol[ti]] >= MyColRot.firstcol[ti] + MyColRot.ncol[ti]) tColRot[tj + MyColRot.firstcol[ti]] -= MyColRot.ncol[ti];
            }
        }
    }
    for (unsigned int tj = 0; tj < MycRom.fHeight; tj++)
    {
        for (unsigned int ti = 0; ti < MycRom.fWidth; ti++)
        {
            if (!original)
            {
                UINT8 nodynaset = MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight + tj * MycRom.fWidth + ti];
                if (nodynaset == 255) SetRenderDrawColorv(&pFramePal[3 * tColRot[pFrame[ti + tj * MycRom.fWidth]]], 255);
                else SetRenderDrawColorv(&MycRom.cPal[acFrame * 3 * MycRom.ncColors + 3 * tColRot[MycRom.Dyna4Cols[acFrame * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors + nodynaset * MycRom.noColors + MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + tj * MycRom.fWidth + ti]]]], 255);
            }
            else SetRenderDrawColorv(&originalcolors[3 * pFrame[ti + tj * MycRom.fWidth]], 255);
            if (clipw != 0) RenderDrawPointClip(glfwin, x + ti * zoom, y + tj * zoom, x + clipw - 1, y + cliph - 1, zoom);
            else RenderDrawPoint(glfwin, x + ti * zoom , y + tj * zoom , zoom);
        }
    }
    glEnd();
    if ((original) && (MycRom.CompMaskID[acFrame] < 255))
    {
        SetRenderDrawColor(255, 0, 255, 255);
        Draw_Over_From_Surface(&MycRom.CompMasks[MycRom.CompMaskID[acFrame] * MycRom.fWidth * MycRom.fHeight],0, (float)zoom, true, true);
    }
    else if (!original)
    {
        if (Ident_Pushed)
        {
            SetRenderDrawColor(mselcol, 0, mselcol, 255);
            Draw_Over_From_Surface(&MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight], acDynaSet, (float)zoom, true, false);
        }
        else
        {
            SetRenderDrawColor(mselcol, 0, mselcol, 255);
            Draw_Over_From_Surface(&MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight], 255, (float)zoom, false, true);
            if (Paste_Mode) Draw_Paste_Over(glfwin, x, y, zoom);
            else
            {
                SetRenderDrawColor(0, mselcol, mselcol, 255);
                Draw_Over_From_Surface(Copy_Mask, 0, (float)zoom, true, true);
            }
        }
    }
}

void Draw_Frame_Strip(void)
{
    // Paste the previsouly calculated texture on the client area of the frame strip
    float RTexCoord = (float)ScrW / (float)MonWidth;
    glfwMakeContextCurrent(glfwframestrip);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, TxFrameStrip[acFSText]);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    glColor4f(1, 1, 1, 1);
    glTexCoord2f(0, 0);
    glVertex2i(0, 0);
    glTexCoord2f(RTexCoord, 0);
    glVertex2i(ScrW, 0);
    glTexCoord2f(RTexCoord, 1);
    glVertex2i(ScrW, FRAME_STRIP_HEIGHT);
    glTexCoord2f(0, 0);
    glVertex2i(0, 0);
    glTexCoord2f(RTexCoord, 1);
    glVertex2i(ScrW, FRAME_STRIP_HEIGHT);
    glTexCoord2f(0, 1);
    glVertex2i(0, FRAME_STRIP_HEIGHT);
    glEnd();

}

void Draw_Sprite_Strip(void)
{
    // Paste the previsouly calculated texture on the client area of the frame strip
    float RTexCoord = (float)ScrW2 / (float)MonWidth;
    glfwMakeContextCurrent(glfwspritestrip);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, TxSpriteStrip[acSSText]);
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    glColor4f(1, 1, 1, 1);
    glTexCoord2f(0, 0);
    glVertex2i(0, 0);
    glTexCoord2f(RTexCoord, 0);
    glVertex2i(ScrW2, 0);
    glTexCoord2f(RTexCoord, 1);
    glVertex2i(ScrW2, FRAME_STRIP_HEIGHT2);
    glTexCoord2f(0, 0);
    glVertex2i(0, 0);
    glTexCoord2f(RTexCoord, 1);
    glVertex2i(ScrW2, FRAME_STRIP_HEIGHT2);
    glTexCoord2f(0, 1);
    glVertex2i(0, FRAME_STRIP_HEIGHT2);
    glEnd();

}

void CalcPosSlider(void)
{
    float div = (float)MycRom.nFrames / (float)SliderWidth;
    PosSlider = (int)((float)PreFrameInStrip / div);
}

void Get_Frame_Strip_Line_Color(UINT pos)
{
    // give the color of the 2 lines under the slider according to if there are selected frames in this line and similar frames to the current one
    UINT corframe = (int)((float)(pos - FRAME_STRIP_SLIDER_MARGIN) * (float)MycRom.nFrames / (float)SliderWidth);
    UINT cornextframe = (int)((float)(pos - FRAME_STRIP_SLIDER_MARGIN + 1) * (float)MycRom.nFrames / (float)SliderWidth);
    if (cornextframe == corframe) cornextframe++;
    bool issel = false, isac = false;
    for (UINT ti = corframe; ti < cornextframe; ti++)
    {
        int iFS = isFrameSelected3(ti);
        if (iFS == -2) isac = true;
        if (iFS > -1) issel = true;
    }
    if (isac == true)
    {
        draw_color[0] = acColor[0];
        draw_color[1] = acColor[1];
        draw_color[2] = acColor[2];
    }
    else if (issel == true)
    {
        draw_color[0] = SelColor[0];
        draw_color[1] = SelColor[1];
        draw_color[2] = SelColor[2];
    }
    else
    {
        draw_color[0] = UnselColor[0];
        draw_color[1] = UnselColor[1];
        draw_color[2] = UnselColor[2];
    }
    issel = false;
    for (UINT ti = corframe; ti < cornextframe; ti++)
    {
        if (isSameFrame(ti) > -1) issel = true;
    }
    if (issel == true)
    {
        under_draw_color[0] = SameColor[0];
        under_draw_color[1] = SameColor[1];
        under_draw_color[2] = SameColor[2];
    }
    else
    {
        under_draw_color[0] = UnselColor[0];
        under_draw_color[1] = UnselColor[1];
        under_draw_color[2] = UnselColor[2];
    }
}

void Frame_Strip_Update(void)
{
    glfwMakeContextCurrent(glfwframestrip);
    // Calculate the texture to display on the frame strip
    if (pFrameStrip) memset(pFrameStrip, 0, MonWidth * FRAME_STRIP_HEIGHT * 4);
    // Draw the frames
    if (MycRom.name[0] == 0) return;
    bool doublepixsize = true;
    if (MycRom.fHeight == 64) doublepixsize = false;
    UINT8* pfr, * pcol, * pstrip, * psmem, * psmem2, * pmask, * pfro;
    UINT addrow = ScrW * 4;
    pstrip = pFrameStrip + FS_LMargin * 4 + FRAME_STRIP_H_MARGIN * addrow;
    UINT8 frFrameColor[3] = { 255,255,255 };
    int fwid = 256;
    if (MycRom.name[0])
    {
        if (MycRom.fWidth == 192) fwid = 192;
    }
    for (int ti = 0; ti < (int)NFrameToDraw; ti++)
    {
        if ((PreFrameInStrip + ti < 0) || (PreFrameInStrip + ti >= (int)MycRom.nFrames))
        {
            pstrip += (fwid + FRAME_STRIP_W_MARGIN) * 4;
            continue;
        }
        if (PreFrameInStrip + ti == acFrame)
        {
            frFrameColor[0] = acColor[0];
            frFrameColor[1] = acColor[1];
            frFrameColor[2] = acColor[2];
        }
        else if (isFrameSelected(PreFrameInStrip + ti) >= 0)
        {
            frFrameColor[0] = SelColor[0];
            frFrameColor[1] = SelColor[1];
            frFrameColor[2] = SelColor[2];
        }
        else
        {
            frFrameColor[0] = UnselColor[0];
            frFrameColor[1] = UnselColor[1];
            frFrameColor[2] = UnselColor[2];
        }
        for (int tj = -1; tj < fwid + 1; tj++)
        {
            *(pstrip + tj * 4 + 64 * addrow) = *(pstrip - addrow + tj * 4) = frFrameColor[0];
            *(pstrip + tj * 4 + 64 * addrow + 1) = *(pstrip - addrow + tj * 4 + 1) = frFrameColor[1];
            *(pstrip + tj * 4 + 64 * addrow + 2) = *(pstrip - addrow + tj * 4 + 2) = frFrameColor[2];
            *(pstrip + tj * 4 + 64 * addrow + 3) = *(pstrip - addrow + tj * 4 + 3) = 255;
        }
        for (int tj = 0; tj < 64; tj++)
        {
            *(pstrip + tj * addrow + fwid * 4) = *(pstrip - 4 + tj * addrow) = frFrameColor[0];
            *(pstrip + tj * addrow + fwid * 4 + 1) = *(pstrip - 4 + tj * addrow + 1) = frFrameColor[1];
            *(pstrip + tj * addrow + fwid * 4 + 2) = *(pstrip - 4 + tj * addrow + 2) = frFrameColor[2];
            *(pstrip + tj * addrow + fwid * 4 + 3) = *(pstrip - 4 + tj * addrow + 3) = 255;
        }
        if (Edit_Mode == 0) // if we are in original frame mode
        {
            pfr = &MycRP.oFrames[(PreFrameInStrip + ti) * MycRom.fWidth * MycRom.fHeight];
            pcol = originalcolors;
            pmask = NULL;
            pfro = NULL;
        }
        else // if we are in colorized frame mode
        {
            pfr = &MycRom.cFrames[(PreFrameInStrip + ti) * MycRom.fWidth * MycRom.fHeight];
            pcol = &MycRom.cPal[(PreFrameInStrip + ti) * MycRom.ncColors * 3];
            pmask = &MycRom.DynaMasks[(PreFrameInStrip + ti) * MycRom.fWidth * MycRom.fHeight];
            pfro= &MycRP.oFrames[(PreFrameInStrip + ti) * MycRom.fWidth * MycRom.fHeight];
        }
        psmem = pstrip;
        for (UINT tj = 0; tj < MycRom.fHeight; tj++)
        {
            psmem2 = pstrip;
            for (UINT tk = 0; tk < MycRom.fWidth; tk++)
            {
                if ((Edit_Mode == 1) && ((*pmask) != 255))
                {
                    pstrip[0] = pcol[3 * MycRom.Dyna4Cols[((PreFrameInStrip + ti) * MAX_DYNA_SETS_PER_FRAME + (*pmask)) * MycRom.noColors + (*pfro)]];
                    pstrip[1] = pcol[3 * MycRom.Dyna4Cols[((PreFrameInStrip + ti) * MAX_DYNA_SETS_PER_FRAME + (*pmask)) * MycRom.noColors + (*pfro)] + 1];
                    pstrip[2] = pcol[3 * MycRom.Dyna4Cols[((PreFrameInStrip + ti) * MAX_DYNA_SETS_PER_FRAME + (*pmask)) * MycRom.noColors + (*pfro)] + 2];
                }
                else
                {
                    pstrip[0] = pcol[3 * (*pfr)];
                    pstrip[1] = pcol[3 * (*pfr) + 1];
                    pstrip[2] = pcol[3 * (*pfr) + 2];
                }
                pstrip[3] = 255;
                if (doublepixsize)
                {
                    pstrip[addrow + 4] = pstrip[addrow] = pstrip[4] = pstrip[0];
                    pstrip[addrow + 5] = pstrip[addrow + 1] = pstrip[5] = pstrip[1];
                    pstrip[addrow + 6] = pstrip[addrow + 2] = pstrip[6] = pstrip[2];
                    pstrip[addrow + 7] = pstrip[addrow + 3] = pstrip[7] = 255;
                    pstrip += 4;
                }
                pstrip += 4;
                pfr++;
                if (Edit_Mode == 1)
                {
                    pmask++;
                    pfro++;
                }
            }
            pstrip = psmem2 + addrow;
            if (doublepixsize) pstrip += addrow;
        }
        pstrip = psmem + (fwid + FRAME_STRIP_W_MARGIN) * 4;
    }
    // Draw the frame numbers above the frames
    int presect = -1, acsect;
    for (UINT ti = 0; ti < NFrameToDraw; ti++)
    {
        if (isSameFrame(PreFrameInStrip + ti) == -1) SetRenderDrawColor(128, 128, 128, 255); else SetRenderDrawColorv((UINT8*)SameColor, 255);
        if (PreFrameInStrip + ti < 0) continue;
        if (PreFrameInStrip + ti >= MycRom.nFrames) continue;
        Draw_Raw_Number(PreFrameInStrip + ti, FS_LMargin + ti * (fwid + FRAME_STRIP_W_MARGIN), FRAME_STRIP_H_MARGIN - RAW_DIGIT_H - 3, pFrameStrip, ScrW, FRAME_STRIP_H_MARGIN);
        SetRenderDrawColor(255, 255, 0, 255);
        Draw_Raw_Number(MycRP.FrameDuration[PreFrameInStrip + ti], FS_LMargin + ti * (fwid + FRAME_STRIP_W_MARGIN) + fwid - 5 * RAW_DIGIT_W - 1, FRAME_STRIP_H_MARGIN - RAW_DIGIT_H - 3, pFrameStrip, ScrW, FRAME_STRIP_H_MARGIN);
        acsect = Which_Section(PreFrameInStrip + ti);
        if (acsect != presect)
        {
            SetRenderDrawColorv((UINT8*)SectionColor, 255);
            Draw_Raw_Number(acsect + 1, FS_LMargin + ti * (fwid + FRAME_STRIP_W_MARGIN) + 100, FRAME_STRIP_H_MARGIN - RAW_DIGIT_H - 3, pFrameStrip, ScrW, FRAME_STRIP_H_MARGIN);
        }
        presect = acsect;
    }
    // Draw the slider below the frames
    for (UINT ti = addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 5); ti < addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 7); ti++)
    {
        pFrameStrip[ti] = 50;
    }
    for (UINT ti = FRAME_STRIP_SLIDER_MARGIN; ti < ScrW - FRAME_STRIP_SLIDER_MARGIN; ti++)
    {
        Get_Frame_Strip_Line_Color(ti);
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti] = draw_color[0];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti + 1] = draw_color[1];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti + 2] = draw_color[2];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 1) + 4 * ti + 3] = 255;
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti] = draw_color[0];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti + 1] = draw_color[1];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti + 2] = draw_color[2];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2) + 4 * ti + 3] = 255;
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti] = under_draw_color[0];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti + 1] = under_draw_color[1];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti + 2] = under_draw_color[2];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 1) + 4 * ti + 3] = 255;
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti] = under_draw_color[0];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti + 1] = under_draw_color[1];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti + 2] = under_draw_color[2];
        pFrameStrip[addrow * (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 2) + 4 * ti + 3] = 255;
        //if (isSameFrame(PreFrameInStrip + ti) == -1) SetRenderDrawColor(255, 255, 255, 255); else SetRenderDrawColor(0, 255, 0, 255);
    }
    SliderWidth = ScrW - 2 * FRAME_STRIP_SLIDER_MARGIN;
    CalcPosSlider();
    for (int ti = -5; ti <= 6; ti++)
    {
        for (int tj = 0; tj <= 2; tj++)
        {
            int offset = (FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + ti) * addrow + (FRAME_STRIP_SLIDER_MARGIN + tj + PosSlider) * 4;
            pFrameStrip[offset] = 255;
            pFrameStrip[offset + 1] = 255;
            pFrameStrip[offset + 2] = 255;
            pFrameStrip[offset + 3] = 255;
        }
    }
    SetRenderDrawColor(255, 255, 255, 255);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, TxFrameStrip[!(acFSText)]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ScrW, FRAME_STRIP_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pFrameStrip); //RGBA with 4 bytes alignment for efficiency
    acFSText = !(acFSText); // equivalent to "x xor 1"
}

void Sprite_Strip_Update(void)
{
    glfwMakeContextCurrent(glfwspritestrip);
    // Calculate the texture to display on the sprite strip
    if (pSpriteStrip) memset(pSpriteStrip, 0, MonWidth * FRAME_STRIP_HEIGHT2 * 4);
    if (MycRom.name[0] == 0) return;
    if (MycRom.nSprites == 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, TxSpriteStrip[!(acSSText)]);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ScrW2, FRAME_STRIP_HEIGHT2, GL_RGBA, GL_UNSIGNED_BYTE, pSpriteStrip); //RGBA with 4 bytes alignment for efficiency
        acSSText = !(acSSText); // equivalent to "x xor 1"
        return;
    }
        // Draw the sprites
    int addrow = ScrW2 * 4;
    UINT8* pstrip = pSpriteStrip + SS_LMargin * 4 + FRAME_STRIP_H_MARGIN * addrow;
    UINT8 frFrameColor[3] = { 255,255,255 };
    int fwid = 2 * MAX_SPRITE_SIZE;
    for (int ti = 0; ti < (int)NSpriteToDraw; ti++)
    {
        if ((PreSpriteInStrip + ti < 0) || (PreSpriteInStrip + ti >= (int)MycRom.nSprites))
        {
            pstrip += (fwid + FRAME_STRIP_W_MARGIN) * 4;
            continue;
        }
        if (PreSpriteInStrip + ti == acSprite)
        {
            frFrameColor[0] = acColor[0];
            frFrameColor[1] = acColor[1];
            frFrameColor[2] = acColor[2];
        }
        else
        {
            frFrameColor[0] = UnselColor[0];
            frFrameColor[1] = UnselColor[1];
            frFrameColor[2] = UnselColor[2];
        }
        for (int tj = -1; tj < fwid + 1; tj++) // draw frame
        {
            *(pstrip + tj * 4 + MAX_SPRITE_SIZE * addrow) = *(pstrip - addrow + tj * 4) = frFrameColor[0];
            *(pstrip + tj * 4 + MAX_SPRITE_SIZE * addrow + 1) = *(pstrip - addrow + tj * 4 + 1) = frFrameColor[1];
            *(pstrip + tj * 4 + MAX_SPRITE_SIZE * addrow + 2) = *(pstrip - addrow + tj * 4 + 2) = frFrameColor[2];
            *(pstrip + tj * 4 + MAX_SPRITE_SIZE * addrow + 3) = *(pstrip - addrow + tj * 4 + 3) = 255;
        }
        for (int tj = 0; tj < MAX_SPRITE_SIZE; tj++)
        {
            *(pstrip + tj * addrow + fwid * 4) = *(pstrip - 4 + tj * addrow) = frFrameColor[0];
            *(pstrip + tj * addrow + fwid * 4 + 1) = *(pstrip - 4 + tj * addrow + 1) = frFrameColor[1];
            *(pstrip + tj * addrow + fwid * 4 + 2) = *(pstrip - 4 + tj * addrow + 2) = frFrameColor[2];
            *(pstrip + tj * addrow + fwid * 4 + 3) = *(pstrip - 4 + tj * addrow + 3) = 255;
        }
        UINT16* psp = &MycRom.SpriteDescriptions[(PreSpriteInStrip + ti) * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE];
        UINT8* pcol = &MycRom.cPal[MycRP.Sprite_Col_From_Frame[(PreSpriteInStrip + ti)] * MycRom.ncColors * 3];
        UINT8* pcol2 = originalcolors;
        for (UINT tj = 0; tj < MAX_SPRITE_SIZE; tj++)
        {
            for (UINT tk = 0; tk < MAX_SPRITE_SIZE; tk++)
            {
                if ((psp[tk + tj * MAX_SPRITE_SIZE] & 0x8000) == 0)
                {
                    pstrip[tj * addrow + tk * 4] = pcol2[3 * (UINT8)(psp[tk + tj * MAX_SPRITE_SIZE] >> 8)];
                    pstrip[tj * addrow + tk * 4 + 1] = pcol2[3 * (UINT8)(psp[tk + tj * MAX_SPRITE_SIZE] >> 8) + 1];
                    pstrip[tj * addrow + tk * 4 + 2] = pcol2[3 * (UINT8)(psp[tk + tj * MAX_SPRITE_SIZE] >> 8) + 2];
                    pstrip[tj * addrow + tk * 4 + 3] = 255;
                    pstrip[tj * addrow + (tk + MAX_SPRITE_SIZE) * 4] = pcol[3 * (UINT8)(psp[tk + tj * MAX_SPRITE_SIZE] & 0xff)];
                    pstrip[tj * addrow + (tk + MAX_SPRITE_SIZE) * 4 + 1] = pcol[3 * (UINT8)(psp[tk + tj * MAX_SPRITE_SIZE] & 0xff) + 1];
                    pstrip[tj * addrow + (tk + MAX_SPRITE_SIZE) * 4 + 2] = pcol[3 * (UINT8)(psp[tk + tj * MAX_SPRITE_SIZE] & 0xff) + 2];
                    pstrip[tj * addrow + (tk + MAX_SPRITE_SIZE) * 4 + 3] = 255;
                }
            }
        }
        pstrip += (fwid + FRAME_STRIP_W_MARGIN) * 4;
    }
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, TxSpriteStrip[!(acSSText)]);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ScrW2, FRAME_STRIP_HEIGHT2, GL_RGBA, GL_UNSIGNED_BYTE, pSpriteStrip); //RGBA with 4 bytes alignment for efficiency
    acSSText = !(acSSText); // equivalent to "x xor 1"
}

void Draw_Sprite(void)
{
    if ((MycRom.name[0] == 0) || (MycRom.nSprites == 0)) return;
    glfwMakeContextCurrent(glfwsprites);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D, TxCircle);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    glColor4f(1, 1, 1, 1);
    glVertex2i(MAX_SPRITE_SIZE * sprite_zoom, 0);
    glVertex2i(MAX_SPRITE_SIZE * sprite_zoom + SPRITE_INTERVAL, 0);
    glVertex2i(MAX_SPRITE_SIZE * sprite_zoom + SPRITE_INTERVAL, ScrH2);
    glVertex2i(MAX_SPRITE_SIZE * sprite_zoom, 0);
    glVertex2i(MAX_SPRITE_SIZE * sprite_zoom + SPRITE_INTERVAL, ScrH2);
    glVertex2i(MAX_SPRITE_SIZE * sprite_zoom, ScrH2);
    glEnd();
    glEnable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLES);
    for (UINT tj = 0; tj < MAX_SPRITE_SIZE; tj++)
    {
        for (UINT ti = 0; ti < MAX_SPRITE_SIZE; ti++)
        {
            if ((MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + ti + tj * MAX_SPRITE_SIZE] & 0x8000) == 0) //if the last bit is on (high order byte is 255), the pixel is ignored, out of the sprite
            {
                SetRenderDrawColorv(&originalcolors[(MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + ti + tj * MAX_SPRITE_SIZE] >> 8) * 3], 255);
                RenderDrawPoint(glfwsprites, ti * sprite_zoom, tj * sprite_zoom, sprite_zoom);
                SetRenderDrawColorv(&MycRom.cPal[MycRP.Sprite_Col_From_Frame[acSprite] * 3 * MycRom.ncColors + (MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + ti + tj * MAX_SPRITE_SIZE] & 0xff) * 3], 255);
                RenderDrawPoint(glfwsprites, ti * sprite_zoom + MAX_SPRITE_SIZE * sprite_zoom + SPRITE_INTERVAL, tj * sprite_zoom, sprite_zoom);
            }
        }
    }
    glEnd();
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
    glColor4ub(mselcol, mselcol, mselcol, 255);
    for (UINT tj = 0; tj < MAX_SPRITE_SIZE; tj++)
    {
        for (UINT ti = 0; ti < MAX_SPRITE_SIZE; ti++)
        {
            if ((MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + ti + tj * MAX_SPRITE_SIZE] & 0x8000) != 0) //if the last bit is on (high order byte is 255), the pixel is ignored, out of the sprite
            {
                glVertex2i(ti * sprite_zoom, tj * sprite_zoom);
                glVertex2i((ti + 1) * sprite_zoom, (tj + 1) * sprite_zoom);
                glVertex2i(MAX_SPRITE_SIZE * sprite_zoom + SPRITE_INTERVAL + ti * sprite_zoom, tj * sprite_zoom);
                glVertex2i(MAX_SPRITE_SIZE * sprite_zoom + SPRITE_INTERVAL + (ti + 1) * sprite_zoom, (tj + 1) * sprite_zoom);
            }
        }
    }
    UINT acdetspr = (UINT)SendMessage(GetDlgItem(hwTB2, IDC_DETSPR), CB_GETCURSEL, 0, 0);
    UINT16* psda = &MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS + acdetspr * 4];

   // psda[0] = psda[1] = 2;
   // psda[2] = psda[3] = 8;

    glColor4ub(255, 255, 255, 255);
    if ((*psda) != 0xffff)
    {
        for (UINT16 tj = psda[1]; tj < psda[1] + psda[3]; tj++)
        {
            for (UINT16 ti = psda[0]; ti < psda[0] + psda[2]; ti++)
            {
                Draw_Line((float)(ti * sprite_zoom), (float)(tj * sprite_zoom), (float)((ti + 1) * sprite_zoom + 1), (float)((tj + 1) * sprite_zoom + 1));
                Draw_Line((float)(ti * sprite_zoom), (float)((tj + 1) * sprite_zoom + 1), (float)((ti + 1) * sprite_zoom + 1), (float)(tj * sprite_zoom));
            }
        }
    }
    glEnd();
}

#pragma endregion Window_Tools_And_Drawings

#pragma region Project_File_Functions

void Free_cRP(void)
{
    // Free buffers for MycRP
    if (MycRP.name[0] != 0)
    {
        MycRP.name[0] = 0;
        // Nothing to free for now
    }
    if (MycRP.oFrames)
    {
        free(MycRP.oFrames);
        MycRP.oFrames = NULL;
    }
    if (MycRP.FrameDuration)
    {
        free(MycRP.FrameDuration);
        MycRP.FrameDuration = NULL;
    }
}

void Free_cRom(void)
{
    // Free buffers for the MycRom
    if (MycRom.name[0] != 0)
    {
        MycRom.name[0] = 0;
        if (MycRom.HashCode)
        {
            free(MycRom.HashCode);
            MycRom.HashCode = NULL;
        }
        if (MycRom.CompMaskID)
        {
            free(MycRom.CompMaskID);
            MycRom.CompMaskID = NULL;
        }
        if (MycRom.MovRctID)
        {
            free(MycRom.MovRctID);
            MycRom.MovRctID = NULL;
        }
        if (MycRom.CompMasks)
        {
            free(MycRom.CompMasks);
            MycRom.CompMasks = NULL;
        }
        if (MycRom.MovRcts)
        {
            free(MycRom.MovRcts);
            MycRom.MovRcts = NULL;
        }
        if (MycRom.cPal)
        {
            free(MycRom.cPal);
            MycRom.cPal = NULL;
        }
        if (MycRom.cFrames)
        {
            free(MycRom.cFrames);
            MycRom.cFrames = NULL;
        }
        if (MycRom.DynaMasks)
        {
            free(MycRom.DynaMasks);
            MycRom.DynaMasks = NULL;
        }
        if (MycRom.Dyna4Cols)
        {
            free(MycRom.Dyna4Cols);
            MycRom.Dyna4Cols = NULL;
        }
        if (MycRom.FrameSprites)
        {
            free(MycRom.FrameSprites);
            MycRom.FrameSprites = NULL;
        }
        if (MycRom.TriggerID)
        {
            free(MycRom.TriggerID);
            MycRom.TriggerID = NULL;
        }
        if (MycRom.ColorRotations)
        {
            free(MycRom.ColorRotations);
            MycRom.ColorRotations = NULL;
        }
        if (MycRom.SpriteDescriptions)
        {
            free(MycRom.SpriteDescriptions);
            MycRom.SpriteDescriptions = NULL;
        }
        /*if (MycRom.SpriteDetectDwords)
        {
            free(MycRom.SpriteDetectDwords);
            MycRom.SpriteDetectDwords = NULL;
        }
        if (MycRom.SpriteDetectDwordPos)
        {
            free(MycRom.SpriteDetectDwordPos);
            MycRom.SpriteDetectDwordPos = NULL;
        }*/
        if (MycRom.SpriteDetDwords)
        {
            free(MycRom.SpriteDetDwords);
            MycRom.SpriteDetDwords = NULL;
        }
        if (MycRom.SpriteDetDwordPos)
        {
            free(MycRom.SpriteDetDwordPos);
            MycRom.SpriteDetDwordPos = NULL;
        }
        if (MycRom.SpriteDetAreas)
        {
            free(MycRom.SpriteDetAreas);
            MycRom.SpriteDetAreas = NULL;
        }
    }
}

void Free_Project(void)
{
    Free_cRP();
    Free_cRom();
    InitVariables();
}

void LoadSaveDir(void)
{
    FILE* pfile;
    if (fopen_s(&pfile, "SaveDir.pos", "rb") != 0)
    {
        //cprintf("No save directory file found, using default");
        strcpy_s(MycRP.SaveDir, 260, DumpDir);
        Ask_for_SaveDir = true;
        return;
    }
    fread(MycRP.SaveDir, 1, MAX_PATH, pfile);
    Ask_for_SaveDir = false;
    fclose(pfile);
}

void SaveSaveDir(void)
{
    FILE* pfile;
    if (fopen_s(&pfile, "SaveDir.pos", "wb") != 0)
    {
        cprintf("Error while saving save directory file");
        return;
    }
    fwrite(MycRP.SaveDir, 1, MAX_PATH, pfile);
    fclose(pfile);
}

void SaveWindowPosition(void)
{
    HKEY tKey;
    LSTATUS ls = RegCreateKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\ColorizingDMD", 0, NULL, 0, KEY_WRITE, NULL, &tKey, NULL);
    if (ls == ERROR_SUCCESS)
    {
        WINDOWPLACEMENT wp;
        wp.length = sizeof(WINDOWPLACEMENT);
        GetWindowPlacement(hWnd, &wp);
        RegSetValueExA(tKey, "MAIN_LEFT", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.left, 4);
        RegSetValueExA(tKey, "MAIN_RIGHT", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.right, 4);
        RegSetValueExA(tKey, "MAIN_TOP", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.top, 4);
        RegSetValueExA(tKey, "MAIN_BOTTOM", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.bottom, 4);
        GetWindowPlacement(hSprites, &wp);
        RegSetValueExA(tKey, "SPRITE_LEFT", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.left, 4);
        RegSetValueExA(tKey, "SPRITE_RIGHT", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.right, 4);
        RegSetValueExA(tKey, "SPRITE_TOP", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.top, 4);
        RegSetValueExA(tKey, "SPRITE_BOTTOM", 0, REG_DWORD, (const BYTE*)&wp.rcNormalPosition.bottom, 4);
        RegCloseKey(tKey);
    }
}

void LoadWindowPosition(void)
{
    HKEY tKey;
    LSTATUS ls = RegOpenKeyExA(HKEY_CURRENT_USER, "SOFTWARE\\ColorizingDMD",0,KEY_READ,&tKey);
    if (ls == ERROR_SUCCESS)
    {
        int posx, posy, wid, hei;
        DWORD size = 4;
        RegGetValueA(tKey, NULL, "SPRITE_LEFT", RRF_RT_REG_DWORD, 0, &posx, &size);
        RegGetValueA(tKey, NULL, "SPRITE_TOP", RRF_RT_REG_DWORD, 0, &posy, &size);
        RegGetValueA(tKey, NULL, "SPRITE_RIGHT", RRF_RT_REG_DWORD, 0, &wid, &size);
        wid -= posx - 1;
        RegGetValueA(tKey, NULL, "SPRITE_BOTTOM", RRF_RT_REG_DWORD, 0, &hei, &size);
        hei -= posy - 1;
        SetWindowPos(hSprites, HWND_TOP, posx, posy, wid, hei, SWP_SHOWWINDOW);
        RegGetValueA(tKey, NULL, "MAIN_LEFT", RRF_RT_REG_DWORD, 0, &posx, &size);
        RegGetValueA(tKey, NULL, "MAIN_TOP", RRF_RT_REG_DWORD, 0, &posy, &size);
        RegGetValueA(tKey, NULL, "MAIN_RIGHT", RRF_RT_REG_DWORD, 0, &wid, &size);
        wid -= posx - 1;
        RegGetValueA(tKey, NULL, "MAIN_BOTTOM", RRF_RT_REG_DWORD, 0, &hei, &size);
        hei -= posy - 1;
        SetWindowPos(hWnd, HWND_TOP, posx, posy, wid, hei, SWP_SHOWWINDOW);
        RegCloseKey(tKey);
    }
}

bool ColorizedFrame(UINT nofr)
{
    UINT8* pcfr = &MycRom.cFrames[nofr * MycRom.fHeight * MycRom.fWidth];
    UINT8* pofr = &MycRP.oFrames[nofr * MycRom.fHeight * MycRom.fWidth];
    UINT8* pdyn = &MycRom.DynaMasks[nofr * MycRom.fHeight * MycRom.fWidth];
    for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
    {
        if (((*pcfr) != (*pofr)) || (*pdyn != 255)) return true;
        pcfr++;
        pofr++;
        pdyn++;
    }
    return false;
}
bool Set_Detection_Dwords(void)
{
    UINT32 Dwords[MAX_SPRITE_SIZE * MAX_SPRITE_SIZE];
    UINT16 DwNum[MAX_SPRITE_SIZE * MAX_SPRITE_SIZE];
    for (UINT tm = 0; tm < MycRom.nSprites; tm++)
    {
        //faire le calcul des dwords pour l'ensemble des zones de déctection de chaque sprite
        for (UINT tl = 0; tl < MAX_SPRITE_DETECT_AREAS; tl++)
        {
            memset(DwNum, 0, sizeof(UINT16) * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
            UINT16* pdetarea = &MycRom.SpriteDetAreas[tm * 4 * MAX_SPRITE_DETECT_AREAS + tl * 4];
            if (pdetarea[0] == 0xffff) continue;
            for (UINT16 tj = 0; tj < pdetarea[3]; tj++)
            {
                for (UINT16 ti = 0; ti < pdetarea[2] - 3; ti++)
                {
                    if (!(MycRom.SpriteDescriptions[tm * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + (pdetarea[1] + tj) * MAX_SPRITE_SIZE + pdetarea[0] + ti] & 0x8000) &&
                        !(MycRom.SpriteDescriptions[tm * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + (pdetarea[1] + tj) * MAX_SPRITE_SIZE + pdetarea[0] + ti + 1] & 0x8000) &&
                        !(MycRom.SpriteDescriptions[tm * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + (pdetarea[1] + tj) * MAX_SPRITE_SIZE + pdetarea[0] + ti + 2] & 0x8000) &&
                        !(MycRom.SpriteDescriptions[tm * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + (pdetarea[1] + tj) * MAX_SPRITE_SIZE + pdetarea[0] + ti + 3] & 0x8000))
                    {
                        UINT32 val = (UINT32)((MycRom.SpriteDescriptions[tm * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + (pdetarea[1] + tj) * MAX_SPRITE_SIZE + pdetarea[0] + ti] & 0xff00) >> 8) +
                            (UINT32)(MycRom.SpriteDescriptions[tm * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + (pdetarea[1] + tj) * MAX_SPRITE_SIZE + pdetarea[0] + ti + 1] & 0xff00) +
                            (UINT32)((MycRom.SpriteDescriptions[tm * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + (pdetarea[1] + tj) * MAX_SPRITE_SIZE + pdetarea[0] + ti + 2] & 0xff00) << 8) +
                            (UINT32)((MycRom.SpriteDescriptions[tm * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + (pdetarea[1] + tj) * MAX_SPRITE_SIZE + pdetarea[0] + ti + 3] & 0xff00) << 16);
                        UINT tk = pdetarea[0];
                        UINT tn = pdetarea[1];
                        UINT to = (pdetarea[1] + tj) * MAX_SPRITE_SIZE + pdetarea[0] + ti;
                        while (tk + tn * MAX_SPRITE_SIZE < to)
                        {
                            if (DwNum[tk + tn * MAX_SPRITE_SIZE] != 0)
                            {
                                if (val == Dwords[tk + tn * MAX_SPRITE_SIZE])
                                {
                                    DwNum[tk + tn * MAX_SPRITE_SIZE]++;
                                    break;
                                }
                            }
                            tk++;
                            if (tk == pdetarea[0] + pdetarea[2])
                            {
                                tk = pdetarea[0];
                                tn++;
                            }
                            if (tk + tn * MAX_SPRITE_SIZE == to)
                            {
                                Dwords[to] = val;
                                DwNum[to] = 1;
                            }
                        }
                    }
                }
            }
            UINT16 minval = MAX_SPRITE_SIZE * MAX_SPRITE_SIZE;
            INT16 acpos = -1;
            for (UINT16 tj = 0; tj < pdetarea[3]; tj++)
            {
                for (UINT16 ti = 0; ti < pdetarea[2] - 3; ti++)
                {
                    if (DwNum[(pdetarea[1] + tj) * MAX_SPRITE_SIZE + pdetarea[0] + ti] > 0)
                    {
                        if (DwNum[(pdetarea[1] + tj) * MAX_SPRITE_SIZE + pdetarea[0] + ti] < minval)
                        {
                            minval = DwNum[(pdetarea[1] + tj) * MAX_SPRITE_SIZE + pdetarea[0] + ti];
                            acpos = (pdetarea[1] + tj) * MAX_SPRITE_SIZE + pdetarea[0] + ti;
                        }
                    }
                }
            }
            if (acpos == -1)
            {
                char tbuf[256];
                sprintf_s(tbuf, 256, "The sprite \"%s\" (number %i) has no contiguous 4 pixels in the detection area number %i, it can not be used in our format. Save cancelled!", &MycRP.Sprite_Names[tm * SIZE_SECTION_NAMES], tm + 1, tl + 1);
                MessageBoxA(hWnd, tbuf, "Caution", MB_OK);
                return false;
            }
            MycRom.SpriteDetDwords[tm * MAX_SPRITE_DETECT_AREAS + tl] = Dwords[acpos];
            MycRom.SpriteDetDwordPos[tm * MAX_SPRITE_DETECT_AREAS + tl] = acpos;
        }
    }
    return true;
}

bool Save_cRom(bool autosave)
{
    if (MycRom.name[0] == 0) return true;
    // Calculating the frame hashcodes
    UINT8* pactiveframes = (UINT8*)malloc(MycRom.nFrames);
    if (!pactiveframes)
    {
        cprintf("Can't get memory for active frames. Action canceled");
        return false;
    }
    for (UINT32 ti = 0; ti < MycRom.nFrames; ti++)
    {
        if (MycRom.CompMaskID[ti] == 255) MycRom.HashCode[ti] = crc32_fast(&MycRP.oFrames[ti * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight, MycRom.ShapeCompMode[ti]);
        else MycRom.HashCode[ti] = crc32_fast_mask(&MycRP.oFrames[ti * MycRom.fWidth * MycRom.fHeight], &MycRom.CompMasks[MycRom.CompMaskID[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight, MycRom.ShapeCompMode[ti]);
    }
    // Calculating the sprites detection DWords
    if (!Set_Detection_Dwords()) return false;
    if ((GetKeyState(VK_SHIFT) & 0x8000) || (Ask_for_SaveDir == true))
    {
        BROWSEINFOA bi;
        bi.hwndOwner = hWnd;
        bi.pidlRoot = NULL;
        //char tbuf2[MAX_PATH];
        bi.pszDisplayName = MycRP.SaveDir;
        bi.lpszTitle = "Choose a save directory...";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        bi.lpfn = NULL;
        bi.iImage = 0;
        LPITEMIDLIST piil;
        piil = SHBrowseForFolderA(&bi);
        if (!piil) return false;
        Ask_for_SaveDir = false;
        SHGetPathFromIDListA(piil, MycRP.SaveDir);
        if (MycRP.SaveDir[strlen(MycRP.SaveDir) - 1] != '\\') strcat_s(MycRP.SaveDir, MAX_PATH, "\\");
        CoTaskMemFree(piil);
    }
    // Save the cRom file
    char tbuf[MAX_PATH];
    if (!autosave) sprintf_s(tbuf, MAX_PATH, "%s%s.cROM", MycRP.SaveDir, MycRom.name); else sprintf_s(tbuf, MAX_PATH, "%s%s(auto).cROM", MycRP.SaveDir, MycRom.name);
    FILE* pfile;
    if (fopen_s(&pfile, tbuf, "wb") != 0)
    {
        if (!(GetKeyState(VK_SHIFT) & 0x8000) && (Ask_for_SaveDir == false) && (!autosave))
        {
            BROWSEINFOA bi;
            bi.hwndOwner = hWnd;
            bi.pidlRoot = NULL;
            //char tbuf2[MAX_PATH];
            bi.pszDisplayName = MycRP.SaveDir;
            bi.lpszTitle = "Choose a save directory...";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            bi.lpfn = NULL;
            bi.iImage = 0;
            LPITEMIDLIST piil;
            piil = SHBrowseForFolderA(&bi);
            if (!piil) return false;
            Ask_for_SaveDir = false;
            SHGetPathFromIDListA(piil, MycRP.SaveDir);
            if (MycRP.SaveDir[strlen(MycRP.SaveDir) - 1] != '\\') strcat_s(MycRP.SaveDir, MAX_PATH, "\\");
            CoTaskMemFree(piil);
            sprintf_s(tbuf, MAX_PATH, "%s%s.cROM", MycRP.SaveDir, MycRom.name);
            if (fopen_s(&pfile, tbuf, "wb") != 0)
            {
                AffLastError((char*)"Save_cRom:fopen_s");
                return false;
            }
        }
        else
        {
            AffLastError((char*)"Save_cRom:fopen_s");
            return false;
        }
    }
    // we set to 0 to the content of the cframes where it's dynamic content to avoid keeping original frames values where unneeded
    for (UINT ti = 0; ti < MycRom.nFrames; ti++)
    {
        for (UINT tj = 0; tj < MycRom.fHeight * MycRom.fWidth; tj++)
        {
            if (MycRom.DynaMasks[ti * MycRom.fHeight * MycRom.fWidth + tj] < 255)
                MycRom.cFrames[ti * MycRom.fHeight * MycRom.fWidth + tj] = 0;
        }
    }
    fwrite(MycRom.name, 1, 64, pfile);
    UINT lengthheader = 11 * sizeof(UINT);
    fwrite(&lengthheader, sizeof(UINT), 1, pfile);
    fwrite(&MycRom.fWidth, sizeof(UINT), 1, pfile);
    fwrite(&MycRom.fHeight, sizeof(UINT), 1, pfile);
    fwrite(&MycRom.nFrames, sizeof(UINT), 1, pfile);
    fwrite(&MycRom.noColors, sizeof(UINT), 1, pfile);
    fwrite(&MycRom.ncColors, sizeof(UINT), 1, pfile);
    fwrite(&MycRom.nCompMasks, sizeof(UINT), 1, pfile);
    fwrite(&MycRom.nMovMasks, sizeof(UINT), 1, pfile);
    fwrite(&MycRom.nSprites, sizeof(UINT), 1, pfile);
    fwrite(MycRom.HashCode, sizeof(UINT), MycRom.nFrames, pfile);
    fwrite(MycRom.ShapeCompMode, 1, MycRom.nFrames, pfile);
    fwrite(MycRom.CompMaskID, 1, MycRom.nFrames, pfile);
    fwrite(MycRom.MovRctID, 1, MycRom.nFrames, pfile);
    if (MycRom.nCompMasks) fwrite(MycRom.CompMasks, 1, MycRom.nCompMasks * MycRom.fWidth * MycRom.fHeight, pfile);
    if (MycRom.nMovMasks) fwrite(MycRom.MovRcts, 1, MycRom.nMovMasks * 4, pfile);
    fwrite(MycRom.cPal, 1, MycRom.nFrames * 3 * MycRom.ncColors, pfile);
    fwrite(MycRom.cFrames, 1, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight, pfile);
    fwrite(MycRom.DynaMasks, 1, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight, pfile);
    fwrite(MycRom.Dyna4Cols, 1, MycRom.nFrames * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors, pfile);
    fwrite(MycRom.FrameSprites, 1, MycRom.nFrames * MAX_SPRITES_PER_FRAME, pfile);
    fwrite(MycRom.SpriteDescriptions, sizeof(UINT16), MycRom.nSprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE, pfile);
    for (UINT ti = 0; ti < MycRom.nFrames; ti++)
    {
        pactiveframes[ti] = 1;
        if (MycRP.FrameDuration[ti] >= SKIP_FRAME_DURATION) continue;
        if (ColorizedFrame(ti)) continue;
        pactiveframes[ti] = 0;
    }
    fwrite(pactiveframes, 1, MycRom.nFrames, pfile);
    free(pactiveframes);
    fwrite(MycRom.ColorRotations, 1, 3 * MAX_COLOR_ROTATION * MycRom.nFrames, pfile);
    fwrite(MycRom.SpriteDetDwords, sizeof(UINT32), MycRom.nSprites * MAX_SPRITE_DETECT_AREAS, pfile);
    fwrite(MycRom.SpriteDetDwordPos, sizeof(UINT16), MycRom.nSprites * MAX_SPRITE_DETECT_AREAS, pfile);
    fwrite(MycRom.SpriteDetAreas, sizeof(UINT16), MycRom.nSprites * 4 * MAX_SPRITE_DETECT_AREAS, pfile);
    fwrite(MycRom.TriggerID, sizeof(UINT32), MycRom.nFrames, pfile);
    fclose(pfile);
    HZIP hz;
    if (!autosave)
    {
        sprintf_s(tbuf, MAX_PATH, "%s%s.cRZ", MycRP.SaveDir, MycRom.name);
        ZipCreateFileA(&hz, tbuf, 0);
        char tbuf2[MAX_PATH];
        sprintf_s(tbuf, MAX_PATH, "%s%s.cRom", MycRP.SaveDir, MycRom.name);
        sprintf_s(tbuf2, MAX_PATH, "%s.cRom", MycRom.name);
        ZipAddFileA(hz, tbuf2, tbuf);// tbuf2);
        ZipClose(hz);
    }
    return true;
}

bool Load_cRom(char* name)
{
    // cROM must be loaded before cRP
    Free_cRom();
/*    char tbuf[MAX_PATH];
    sprintf_s(tbuf, MAX_PATH, "%s%s", DumpDir, name);*/
    FILE* pfile;
    if (fopen_s(&pfile, name, "rb") != 0)
    {
        AffLastError((char*)"Load_cRom:fopen_s");
        return false;
    }
    fread(MycRom.name, 1, 64, pfile);
    UINT lengthheader;
    fread(&lengthheader, sizeof(UINT), 1, pfile);
    fread(&MycRom.fWidth, sizeof(UINT), 1, pfile);
    fread(&MycRom.fHeight, sizeof(UINT), 1, pfile);
    fread(&MycRom.nFrames, sizeof(UINT), 1, pfile);
    fread(&MycRom.noColors, sizeof(UINT), 1, pfile);
    fread(&MycRom.ncColors, sizeof(UINT), 1, pfile);
    fread(&MycRom.nCompMasks, sizeof(UINT), 1, pfile);
    fread(&MycRom.nMovMasks, sizeof(UINT), 1, pfile);
    fread(&MycRom.nSprites, sizeof(UINT), 1, pfile);
    MycRom.HashCode = (UINT*)malloc(sizeof(UINT) * MycRom.nFrames);
    MycRom.ShapeCompMode = (UINT8*)malloc(MycRom.nFrames);
    MycRom.CompMaskID = (UINT8*)malloc(MycRom.nFrames);
    MycRom.MovRctID = (UINT8*)malloc(MycRom.nFrames);
    MycRom.CompMasks = (UINT8*)malloc(MAX_MASKS * MycRom.fWidth * MycRom.fHeight);
    MycRom.MovRcts = (UINT8*)malloc(MycRom.nMovMasks * 4);
    MycRom.cPal = (UINT8*)malloc(MycRom.nFrames * 3 * MycRom.ncColors);
    MycRom.cFrames = (UINT8*)malloc(MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    MycRom.DynaMasks = (UINT8*)malloc(MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    MycRom.Dyna4Cols = (UINT8*)malloc(MycRom.nFrames * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors);
    MycRom.FrameSprites = (UINT8*)malloc(MycRom.nFrames * MAX_SPRITES_PER_FRAME);
    MycRom.SpriteDescriptions = (UINT16*)malloc(MycRom.nSprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE * sizeof(UINT16));
    MycRom.ColorRotations = (UINT8*)malloc(MycRom.nFrames * 3 * MAX_COLOR_ROTATION);
    MycRom.SpriteDetDwords = (UINT32*)malloc(MycRom.nSprites * sizeof(UINT32)*MAX_SPRITE_DETECT_AREAS);
    MycRom.SpriteDetDwordPos = (UINT16*)malloc(MycRom.nSprites * sizeof(UINT16) * MAX_SPRITE_DETECT_AREAS);
    MycRom.SpriteDetAreas = (UINT16*)malloc(MycRom.nSprites * sizeof(UINT16) * MAX_SPRITE_DETECT_AREAS * 4);
    MycRom.TriggerID = (UINT32*)malloc(MycRom.nFrames * sizeof(UINT32));
    if ((!MycRom.ShapeCompMode) || (!MycRom.HashCode) || (!MycRom.CompMaskID) || (!MycRom.MovRctID) || (!MycRom.CompMasks) ||
        (!MycRom.MovRcts) || (!MycRom.cPal) || (!MycRom.cFrames) || (!MycRom.DynaMasks) || (!MycRom.Dyna4Cols) || (!MycRom.FrameSprites) ||
        (!MycRom.SpriteDescriptions) ||
        (!MycRom.ColorRotations) || (!MycRom.SpriteDetDwords) || (!MycRom.SpriteDetDwordPos) || (!MycRom.SpriteDetAreas) || (!MycRom.TriggerID))
    {
        cprintf("Can't get the buffers in Load_cRom");
        Free_cRom(); // We free the buffers we got
        fclose(pfile);
        return false;
    }
    memset(MycRom.CompMasks, 0, MAX_MASKS * MycRom.fWidth * MycRom.fHeight);
    fread(MycRom.HashCode, sizeof(UINT), MycRom.nFrames, pfile);
    fread(MycRom.ShapeCompMode, 1, MycRom.nFrames, pfile);
    fread(MycRom.CompMaskID, 1,  MycRom.nFrames, pfile);
    fread(MycRom.MovRctID, 1, MycRom.nFrames, pfile);
    if (MycRom.nCompMasks) fread(MycRom.CompMasks, 1, MycRom.nCompMasks * MycRom.fWidth * MycRom.fHeight, pfile);
    if (MycRom.nMovMasks) fread(MycRom.MovRcts, 1, MycRom.nMovMasks * 4, pfile);
    fread(MycRom.cPal, 1, MycRom.nFrames * 3 * MycRom.ncColors, pfile);
    fread(MycRom.cFrames, 1, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight, pfile);
    fread(MycRom.DynaMasks, 1, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight, pfile);
    fread(MycRom.Dyna4Cols, 1, MycRom.nFrames * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors, pfile);
    fread(MycRom.FrameSprites, 1, MycRom.nFrames * MAX_SPRITES_PER_FRAME, pfile);
    fread(MycRom.SpriteDescriptions, sizeof(UINT16), MycRom.nSprites * MAX_SPRITE_SIZE* MAX_SPRITE_SIZE, pfile);
    fseek(pfile, MycRom.nFrames, SEEK_CUR); // we skip the active frame content
    memset(MycRom.ColorRotations, 255, 3 * MAX_COLOR_ROTATION * MycRom.nFrames);
    memset(MycRom.SpriteDetAreas, 255, sizeof(UINT16) * 4 * MAX_SPRITE_DETECT_AREAS * MycRom.nSprites);
    memset(MycRom.TriggerID, 0xff, sizeof(UINT32) * MycRom.nFrames);
    if (lengthheader >= 9 * sizeof(UINT))
    {
        fread(MycRom.ColorRotations, 1, 3 * MAX_COLOR_ROTATION * MycRom.nFrames, pfile);
        if (lengthheader >= 10 * sizeof(UINT))
        {
            fread(MycRom.SpriteDetDwords, sizeof(UINT32), MycRom.nSprites * MAX_SPRITE_DETECT_AREAS, pfile);
            fread(MycRom.SpriteDetDwordPos, sizeof(UINT16), MycRom.nSprites * MAX_SPRITE_DETECT_AREAS, pfile);
            fread(MycRom.SpriteDetAreas, sizeof(UINT16), MycRom.nSprites * 4 * MAX_SPRITE_DETECT_AREAS, pfile);
            if (lengthheader >= 11 * sizeof(UINT))
            {
                fread(MycRom.TriggerID, sizeof(UINT32), MycRom.nFrames, pfile);
            }
            memset(MycRom.TriggerID, 0xff, sizeof(UINT32) * MycRom.nFrames);
        }
    }
    fclose(pfile);
    return true;
}

bool Save_cRP(bool autosave)
{
    if (MycRP.name[0] == 0) return true;
    char tbuf[MAX_PATH];
    if (!autosave) sprintf_s(tbuf, MAX_PATH, "%s%s.cRP", MycRP.SaveDir, MycRP.name); else sprintf_s(tbuf, MAX_PATH, "%s%s(auto).cRP", MycRP.SaveDir, MycRP.name);
    FILE* pfile;
    if (fopen_s(&pfile, tbuf, "wb") != 0)
    {
        AffLastError((char*)"Save_cRP:fopen_s");
        return false;
    }
    fwrite(MycRP.name, 1, 64, pfile);
    fwrite(MycRP.oFrames, 1, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight, pfile);
    fwrite(MycRP.activeColSet, sizeof(BOOL), MAX_COL_SETS, pfile);
    fwrite(MycRP.ColSets, sizeof(UINT8), MAX_COL_SETS * 16, pfile);
    fwrite(&MycRP.acColSet, sizeof(UINT8), 1, pfile);
    fwrite(&MycRP.preColSet, sizeof(UINT8), 1, pfile);
    fwrite(MycRP.nameColSet, sizeof(char), MAX_COL_SETS * 64, pfile);
    fwrite(&MycRP.DrawColMode, sizeof(UINT32), 1, pfile);
    fwrite(&MycRP.Draw_Mode, sizeof(UINT8), 1, pfile);
    fwrite(&MycRP.Mask_Sel_Mode, sizeof(int), 1, pfile);
    fwrite(&MycRP.Fill_Mode, sizeof(BOOL), 1, pfile);
    fwrite(MycRP.Mask_Names, sizeof(char), MAX_MASKS * SIZE_MASK_NAME, pfile);
    fwrite(&MycRP.nSections, sizeof(UINT32), 1, pfile);
    fwrite(MycRP.Section_Firsts, sizeof(UINT32), MAX_SECTIONS, pfile);
    fwrite(MycRP.Section_Names, sizeof(char), MAX_SECTIONS * SIZE_SECTION_NAMES, pfile);
    fwrite(MycRP.Sprite_Names, sizeof(char), 255 * SIZE_SECTION_NAMES, pfile);
    fwrite(MycRP.Sprite_Col_From_Frame, sizeof(UINT32), 255, pfile);
    fwrite(MycRP.FrameDuration, sizeof(UINT32), MycRom.nFrames, pfile);
    fwrite(MycRP.Sprite_Edit_Colors, 1, 16 * 255, pfile);
    fwrite(MycRP.SaveDir, 1, 260, pfile);
    fwrite(MycRP.SpriteRect, sizeof(UINT16), 4 * 255, pfile);
    fwrite(MycRP.SpriteRectMirror, sizeof(BOOL), 2 * 255, pfile);
    fclose(pfile);
    return true;
}

bool Load_cRP(char* name)
{
    // cRP must be loaded after cROM
    Free_cRP();
    //char tbuf[MAX_PATH];
    //sprintf_s(tbuf, MAX_PATH, "%s%s", DumpDir, name);
    FILE* pfile;
    if (fopen_s(&pfile, name, "rb") != 0)
    {
        AffLastError((char*)"Load_cRP:fopen_s");
        return false;
    }
    MycRP.oFrames = (UINT8*)malloc(MycRom.nFrames * MycRom.fWidth * MycRom.fHeight);
    if (!MycRP.oFrames)
    {
        cprintf("Can't get the buffer in Load_cRP");
        Free_cRP(); // We free the buffers we got
        fclose(pfile);
        return false;
    }
    MycRP.FrameDuration = (UINT32*)malloc(MycRom.nFrames * sizeof(UINT32));
    if (!MycRP.FrameDuration)
    {
        cprintf("Can't get the buffer in Load_cRP");
        Free_cRP(); // We free the buffers we got
        fclose(pfile);
        return false;
    }
    fread(MycRP.name, 1, 64, pfile);
    fread(MycRP.oFrames, 1, MycRom.nFrames * MycRom.fWidth * MycRom.fHeight, pfile);
    fread(MycRP.activeColSet, sizeof(BOOL), MAX_COL_SETS, pfile);
    fread(MycRP.ColSets, sizeof(UINT8), MAX_COL_SETS * 16, pfile);
    fread(&MycRP.acColSet, sizeof(UINT8), 1, pfile);
    fread(&MycRP.preColSet, sizeof(UINT8), 1, pfile);
    fread(MycRP.nameColSet, sizeof(char), MAX_COL_SETS * 64, pfile);
    fread(&MycRP.DrawColMode, sizeof(UINT32), 1, pfile);
    if (MycRP.DrawColMode == 2) MycRP.DrawColMode = 0;
    fread(&MycRP.Draw_Mode, sizeof(UINT8), 1, pfile);
    fread(&MycRP.Mask_Sel_Mode, sizeof(int), 1, pfile);
    fread(&MycRP.Fill_Mode, sizeof(BOOL), 1, pfile);
    fread(MycRP.Mask_Names, sizeof(char), MAX_MASKS * SIZE_MASK_NAME, pfile);
    fread(&MycRP.nSections, sizeof(UINT32), 1, pfile);
    fread(MycRP.Section_Firsts, sizeof(UINT32), MAX_SECTIONS, pfile);
    fread(MycRP.Section_Names, sizeof(char), MAX_SECTIONS * SIZE_SECTION_NAMES, pfile);
    fread(MycRP.Sprite_Names, sizeof(char), 255 * SIZE_SECTION_NAMES, pfile);
    fread(MycRP.Sprite_Col_From_Frame, sizeof(UINT32), 255, pfile);
    for (UINT ti = 0; ti < MycRom.nFrames; ti++) MycRP.FrameDuration[ti] = 0;
    fread(MycRP.FrameDuration, sizeof(UINT32), MycRom.nFrames, pfile);
    fread(MycRP.Sprite_Edit_Colors, 1, 16 * 255, pfile);
    fread(MycRP.SaveDir, 1, 260, pfile);
    memset(MycRP.SpriteRect, 255, sizeof(UINT16) * 4 * 255);
    fread(MycRP.SpriteRect, sizeof(UINT16), 4 * 255, pfile);
    fread(MycRP.SpriteRectMirror, sizeof(BOOL), 2 * 255, pfile);
    fclose(pfile);
    /*
    char tbuf[MAX_PATH];
    sprintf_s(tbuf, MAX_PATH, "%s%s.ouf", DumpDir, MycRom.name);
    if (fopen_s(&pfile, tbuf, "rb") != 0)
    {
        AffLastError((char*)"Load_ouf:fopen_s");
        return false;
    }
    fread(&MycRom.nSprites, sizeof(UINT), 1, pfile);
    fread(MycRom.SpriteDescriptions, sizeof(UINT16), MycRom.nSprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE, pfile);
    fread(MycRP.Sprite_Names, sizeof(char), 255 * SIZE_SECTION_NAMES, pfile);
    fread(MycRP.Sprite_Col_From_Frame, sizeof(UINT32), 255, pfile);
    fread(MycRP.Sprite_Edit_Colors, 1, 16 * 255, pfile);
    fclose(pfile);
    */
    return true;
}

#pragma endregion Project_File_Functions

#pragma region Txt_File_Operations

unsigned int Count_TXT_Frames(char* TXTF_buffer,size_t TXTF_buffer_len)
{
    // check the number of frames in the txt file
    unsigned int nF = 0;
    for (size_t curPos = 0; curPos < TXTF_buffer_len; curPos++)
    {
        if (TXTF_buffer[curPos] == 'x') nF++;
    }
    return nF;
}

bool Get_Frames_Ptr_Size_And_Number_Of_Colors(sFrames** ppFrames,UINT nFrames,char* TXTF_buffer,size_t TXTF_buffer_len)
{
    // we get the pointer on the data of each frame inside the txt file
    MycRom.noColors = 4;
    sFrames* pFrames = (sFrames*)malloc(sizeof(sFrames) * nFrames);
    *ppFrames = pFrames;
    //char tbuf[16];
    if (!pFrames)
    {
        cprintf("Unable to get buffer memory for the frames");
        return false;
    }
    unsigned int acFr = 0;
    for (size_t curPos = 0; curPos < TXTF_buffer_len; curPos++)
    {
        if (TXTF_buffer[curPos] == 'x')
        {
            char tbuf[12];
            tbuf[0] = '0';
            for (UINT ti = 0; ti < 9; ti++) tbuf[ti + 1] = TXTF_buffer[curPos + ti];
            tbuf[10] = 0;
            pFrames[acFr].timecode = (UINT32)strtol(tbuf, NULL, 16);
            // next line
            while ((TXTF_buffer[curPos] != '\n') && (TXTF_buffer[curPos] != '\r')) curPos++;
            while ((TXTF_buffer[curPos] == '\n') || (TXTF_buffer[curPos] == '\r') || (TXTF_buffer[curPos] == ' ')) curPos++;
            // we are at the beginning of the frame
            pFrames[acFr].active = TRUE;
            pFrames[acFr].ptr = &TXTF_buffer[curPos];
            // if first frame, we check the size of the frame
            if (acFr == 0)
            {
                size_t finPos = curPos;
                while (TXTF_buffer[finPos] > ' ') finPos++;
                MycRom.fWidth = (unsigned int)(finPos - curPos);
                while (TXTF_buffer[finPos + 1] <= ' ') finPos++;
                MycRom.fHeight = 1;
                while (TXTF_buffer[finPos + 1] > ' ')
                {
                    finPos += 2;
                    while (TXTF_buffer[finPos] != '\n') finPos++;
                    MycRom.fHeight++;
                }
            }
            // then we translate the frame '0'-'9' -> 0-9, 'a'-'f' -> 10-15, 'A'-'F' -> 10-15 and removing the '\r' and '\n' and check for the number of colors
            char* pPos = pFrames[acFr].ptr;
            char* pPos2 = pPos;
            for (UINT tj = 0; tj < MycRom.fHeight; tj++)
            {
                for (UINT ti = 0; ti < MycRom.fWidth; ti++)
                {
                    if ((*pPos >= (UINT8)'0') && (*pPos <= (UINT8)'9')) *pPos2 = *pPos - (UINT8)'0';
                    else if ((*pPos >= (UINT8)'A') && (*pPos <= (UINT8)'Z')) *pPos2 = *pPos - (UINT8)'A' + 10;
                    else if ((*pPos >= (UINT8)'a') && (*pPos <= (UINT8)'z')) *pPos2 = *pPos - (UINT8)'a' + 10;
                    if ((*pPos2) > 3) MycRom.noColors = 16;
                    pPos++;
                    pPos2++;
                }
                while (((*pPos) == '\n') || ((*pPos) == '\r')) pPos++;
            }
            curPos = pPos - TXTF_buffer;
            acFr++;
        }
    }
    //if (MycRom.fHeight == 64) acZoom = basezoom; else acZoom = 2 * basezoom;
    return true;
}

bool Parse_TXT(char* TXTF_name, char* TXTF_buffer, size_t TXTF_buffer_len, sFrames** ppFrames, UINT* pnFrames)
{
    // Initial TXT file parsing: count the frames in the file then get pointers to the frames in the file buffer + determine nb of colors and frame size
    if (!TXTF_buffer) return false;
    *pnFrames = Count_TXT_Frames(TXTF_buffer, TXTF_buffer_len);
    if (!Get_Frames_Ptr_Size_And_Number_Of_Colors(ppFrames, *pnFrames, TXTF_buffer, TXTF_buffer_len))
    {
        Free_Project();
        return false;
    }
    cprintf("Opened txt file %s with %i frames: resolution %ix%i, %i colors", TXTF_name, *pnFrames,MycRom.fWidth,MycRom.fHeight,MycRom.noColors);
    return true;
}

void CompareFrames(UINT nFrames, sFrames* pFrames)
{
    // We have a block of sFrames with pointers to frames decoded ('a'->10, '1'->1, etc..., and with no CR and LF), active is set to TRUE and
    // the timecode is the value from the TXT file
    // we need to filter the frames according what has been checked in the IID_FILTERS dialog and to change the value of timecode so that this is the
    // time span the frame has been displayed

    UINT nfremoved = 0;
    UINT nfrremtime = 0, nfrremcol = 0, nfrremsame = 0;
    for (int ti = 0; ti < (int)nFrames; ti++)
    {
        if (!(ti % 200)) Display_Avancement((float)ti / (float)(nFrames - 1), 0, 2);
        if (ti < (int)nFrames - 1)
        {
            UINT32 nextfrlen = pFrames[ti + 1].timecode;
            UINT32 acfrlen = pFrames[ti].timecode;
            if (nextfrlen < acfrlen) pFrames[ti].timecode = DEFAULT_FRAME_DURATION;
            else if (nextfrlen > acfrlen + MAX_FRAME_DURATION) pFrames[ti].timecode = DEFAULT_FRAME_DURATION;
            else
            {
                pFrames[ti].timecode = nextfrlen - acfrlen;
                if (filter_time && (pFrames[ti].timecode < (UINT32)filter_length) && pFrames[ti].active)
                {
                    nfrremtime++;
                    nfremoved++;
                    pFrames[ti].active = FALSE;
                    continue;
                }
            }
        }
        else pFrames[ti].timecode = DEFAULT_FRAME_DURATION;
        // we check the number of colors in each frame if needed and do the no-mask-hash calculations at the same time
        UINT8 ncols;
        pFrames[ti].hashcode = crc32_fast_count((UINT8*)pFrames[ti].ptr, MycRom.fWidth * MycRom.fHeight, FALSE, &ncols);
        if (filter_color && (filter_ncolor < ncols) && pFrames[ti].active)
        {
            nfrremcol++;
            nfremoved++;
            pFrames[ti].active = FALSE;
            continue;
        }
    }
    if (nFrames < 2) return;
    for (int ti = 0; ti < (int)nFrames - 1; ti++)
    {
        if (!(ti%200)) Display_Avancement((float)ti / (float)(nFrames - 1), 1, 2);
        if (pFrames[ti].active == FALSE) continue;
        for (int tj = ti + 1; tj < (int)nFrames; tj++)
        {
            if (pFrames[tj].active == FALSE) continue;
            if (pFrames[ti].hashcode == pFrames[tj].hashcode)
            {
                nfrremsame++;
                nfremoved++;
                pFrames[tj].active = FALSE;
            }
        }
    }
    cprintf("%i frames removed (%i for short duration, %i for too many colors, %i identical), %i added", nfremoved, nfrremtime, nfrremcol, nfrremsame, nFrames - nfremoved);
}

void CompareAdditionalFrames(UINT nFrames, sFrames* pFrames)
{
    //HWND hDlg = Display_Wait("Please wait...");
    // Compare the new frames of a txt file to the previous and new ones to remove copy frames
    //unsigned int nfkilled = 0;
    UINT nfremoved = 0;
    UINT nfrremtime = 0, nfrremcol = 0, nfrremmask = 0, nfrremsame = 0;
    // first compare the new frames between them
    for (int ti = 0; ti < (int)nFrames; ti++)
    {
        if (!(ti % 200)) Display_Avancement((float)ti / (float)(nFrames - 1), 0, 4);
        if (ti < (int)nFrames - 1)
        {
            UINT32 nextfrlen = pFrames[ti + 1].timecode;
            UINT32 acfrlen = pFrames[ti].timecode;
            if (nextfrlen < acfrlen) pFrames[ti].timecode = DEFAULT_FRAME_DURATION;
            else if (nextfrlen > acfrlen + MAX_FRAME_DURATION) pFrames[ti].timecode = DEFAULT_FRAME_DURATION;
            else
            {
                pFrames[ti].timecode = nextfrlen - acfrlen;
                if (filter_time && (pFrames[ti].timecode < (UINT32)filter_length) && pFrames[ti].active)
                {
                    nfrremtime++;
                    nfremoved++;
                    pFrames[ti].active = FALSE;
                    continue;
                }
            }
        }
        else pFrames[ti].timecode = DEFAULT_FRAME_DURATION;
        // we check the number of colors in each frame if needed and do the no-mask-hash calculations at the same time
        UINT8 ncols;
        pFrames[ti].hashcode = crc32_fast_count((UINT8*)pFrames[ti].ptr, MycRom.fWidth * MycRom.fHeight, FALSE, &ncols);
        if (filter_color && (filter_ncolor < ncols) && pFrames[ti].active)
        {
            nfrremcol++;
            nfremoved++;
            pFrames[ti].active = FALSE;
            continue;
        }
    }
    if (nFrames < 2) return;
    for (int ti = 0; ti < (int)nFrames - 1; ti++)
    {
        if (!(ti % 200)) Display_Avancement((float)ti / (float)(nFrames - 1), 1, 4);
        if (pFrames[ti].active == FALSE) continue;
        for (int tj = ti + 1; tj < (int)nFrames; tj++)
        {
            if (pFrames[tj].active == FALSE) continue;
            if (pFrames[ti].hashcode == pFrames[tj].hashcode)
            {
                nfrremsame++;
                nfremoved++;
                pFrames[tj].active = FALSE;
            }
        }
    }
    // calculate the hashes for the old frames
    UINT32* pnomaskhash = (UINT32*)malloc(sizeof(UINT32) * MycRom.nFrames);
    if (!pnomaskhash)
    {
        MessageBoxA(hWnd, "Impossible to get memory for flat hashes, incomplete comparison!", "Error", MB_OK);
        return;
    }
    for (int ti = 0; ti < (int)MycRom.nFrames; ti++)
    {
        if (!(ti % 200)) Display_Avancement((float)ti / (float)(MycRom.nFrames - 1), 2, 4);
        if (MycRom.CompMaskID[ti] != 255)
        {
            MycRom.HashCode[ti] = crc32_fast_mask(&MycRP.oFrames[MycRom.fWidth * MycRom.fHeight * ti], &MycRom.CompMasks[MycRom.CompMaskID[ti] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight, (BOOL)MycRom.ShapeCompMode[ti]);
            pnomaskhash[ti] = crc32_fast(&MycRP.oFrames[MycRom.fWidth * MycRom.fHeight * ti], MycRom.fWidth * MycRom.fHeight, FALSE);
        }
        else
        {
            MycRom.HashCode[ti] = crc32_fast(&MycRP.oFrames[MycRom.fWidth * MycRom.fHeight * ti], MycRom.fWidth * MycRom.fHeight, (BOOL)MycRom.ShapeCompMode[ti]);
            if (MycRom.ShapeCompMode[ti] == FALSE) pnomaskhash[ti] = MycRom.HashCode[ti];
            else pnomaskhash[ti] = crc32_fast(&MycRP.oFrames[MycRom.fWidth * MycRom.fHeight * ti], MycRom.fWidth * MycRom.fHeight, FALSE);
        }
    }
    // then compare the new frames with the previous ones
    for (unsigned int ti = 0; ti < nFrames; ti++)
    {
        if (pFrames[ti].active == FALSE) continue;
        if (!(ti % 200)) Display_Avancement((float)ti / (float)(nFrames - 1), 3, 4);
        UINT8 premask = 255;
        BOOL isshapemode = FALSE;
        UINT32 achash = crc32_fast((UINT8*)pFrames[ti].ptr, MycRom.fWidth * MycRom.fHeight, FALSE);
        for (int tj = 0; tj < (int)MycRom.nFrames; tj++)
        {
            if ((MycRom.CompMaskID[tj] != 255) && filter_allmask)
            {
                if ((premask != MycRom.CompMaskID[tj]) || (isshapemode != (BOOL)MycRom.ShapeCompMode[tj]))
                {
                    achash = crc32_fast_mask((UINT8*)pFrames[ti].ptr, &MycRom.CompMasks[MycRom.CompMaskID[tj] * MycRom.fWidth * MycRom.fHeight], MycRom.fWidth * MycRom.fHeight, (BOOL)MycRom.ShapeCompMode[tj]);
                    premask = MycRom.CompMaskID[tj];
                    isshapemode = (BOOL)MycRom.ShapeCompMode[tj];
                }
                if (MycRom.HashCode[tj] == achash)
                {
                    pFrames[ti].active = FALSE;
                    nfrremmask++;
                    nfremoved++;
                    break;
                }
            }
            else if (filter_allmask && (MycRom.ShapeCompMode[tj] == TRUE))
            {
                if ((premask != 255) || (isshapemode == FALSE))
                {
                    achash = crc32_fast((UINT8*)pFrames[ti].ptr, MycRom.fWidth * MycRom.fHeight, TRUE);
                    premask = 255;
                    isshapemode = TRUE;
                }
                if (MycRom.HashCode[tj] == achash)
                {
                    pFrames[ti].active = FALSE;
                    nfrremmask++;
                    nfremoved++;
                    break;
                }
            }
            else
            {
                if (pnomaskhash[tj] == pFrames[ti].hashcode)
                {
                    pFrames[ti].active = FALSE;
                    nfrremsame++;
                    nfremoved++;
                    break;
                }
            }
        }
    }
    free(pnomaskhash);
    cprintf("%i frames removed (%i for short duration, %i for too many colors, %i identical with mask and/or shapemode, %i identical) %i added", nfremoved, nfrremtime, nfrremcol, nfrremmask, nfrremsame, nFrames - nfremoved);
}

bool CopyTXTFrames2Frame(UINT nFrames, sFrames* pFrames)
{
    // Convert a txt frame to oFrame
    MycRom.ncColors = 64;
    // count remaining original frames
    unsigned int nF = 0;
    for (unsigned int ti = 0; ti < nFrames; ti++)
    {
        if (pFrames[ti].active == TRUE) nF++;
    }
    // allocating cROM Frame space
    MycRP.oFrames = (UINT8*)malloc(nF * sizeof(UINT8) * MycRom.fWidth * MycRom.fHeight);
    if (!MycRP.oFrames)
    {
        cprintf("Unable to allocate memory for original frames");
        return false;
    }
    MycRom.HashCode = (UINT*)malloc(nF * sizeof(UINT));
    if (!MycRom.HashCode)
    {
        Free_cRom();
        cprintf("Unable to allocate memory for hashtags");
        return false;
    }
    MycRom.ShapeCompMode = (UINT8*)malloc(nF);
    if (!MycRom.ShapeCompMode)
    {
        Free_cRom();
        cprintf("Unable to allocate memory for shape mode");
        return false;
    }
    memset(MycRom.ShapeCompMode, FALSE, nF);
    MycRom.CompMaskID = (UINT8*)malloc(nF * sizeof(UINT8));
    if (!MycRom.CompMaskID)
    {
        Free_cRom();
        cprintf("Unable to allocate memory for comparison mask IDs");
        return false;
    }
    memset(MycRom.CompMaskID, 255, nF * sizeof(UINT8));
    MycRom.MovRctID = (UINT8*)malloc(nF * sizeof(UINT8));
    if (!MycRom.MovRctID)
    {
        Free_cRom();
        cprintf("Unable to allocate memory for moving comparison mask IDs");
        return false;
    }
    memset(MycRom.MovRctID, 255, nF * sizeof(UINT8));
    MycRom.CompMasks = (UINT8*)malloc(MAX_MASKS * MycRom.fWidth * MycRom.fHeight);
    if (!MycRom.CompMasks)
    {
        cprintf("Unable to allocate memory for comparison masks");
        return false;
    }
    memset(MycRom.CompMasks, 0, MAX_MASKS * MycRom.fWidth * MycRom.fHeight);
    MycRom.MovRcts = NULL;
    MycRom.nCompMasks = 0;
    MycRom.nMovMasks = 0;
    MycRom.nSprites = 0;
    size_t sizepalette = MycRom.ncColors * 3;
    MycRom.cPal = (UINT8*)malloc(nF * sizepalette);
    if (!MycRom.cPal)
    {
        Free_cRom();
        cprintf("Unable to allocate memory for colorized palettes");
        return false;
    }
    MycRom.cFrames = (UINT8*)malloc(nF * (size_t)MycRom.fWidth * (size_t)MycRom.fHeight);
    if (!MycRom.cFrames)
    {
        cprintf("Unable to allocate memory for colorized frames");
        Free_cRom();
        return false;
    }
    MycRom.DynaMasks = (UINT8*)malloc(nF * (size_t)MycRom.fWidth * (size_t)MycRom.fHeight);
    if (!MycRom.DynaMasks)
    {
        cprintf("Unable to allocate memory for dynamic masks");
        Free_cRom();
        return false;
    }
    memset(MycRom.DynaMasks, 255, nF * MycRom.fWidth * MycRom.fHeight);
    MycRom.Dyna4Cols = (UINT8*)malloc(nF * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors);
    if (!MycRom.Dyna4Cols)
    {
        cprintf("Unable to allocate memory for dynamic color sets");
        Free_cRom();
        return false;
    }
    for (UINT tj = 0; tj < nF; tj++)
    {
        for (UINT ti = 0; ti < MAX_DYNA_SETS_PER_FRAME; ti++)
        {
            for (UINT tk = 0; tk < MycRom.noColors; tk++) MycRom.Dyna4Cols[tj * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors + ti * MycRom.noColors + tk] = (UINT8)((ti * MycRom.noColors + tk) % 64);
        }
    }
    MycRom.FrameSprites = (UINT8*)malloc(nF * MAX_SPRITES_PER_FRAME);
    if (!MycRom.FrameSprites)
    {
        cprintf("Unable to allocate memory for sprite IDs");
        Free_cRom();
        return false;
    }
    memset(MycRom.FrameSprites, 255, nF * MAX_SPRITES_PER_FRAME);
    MycRom.TriggerID = (UINT32*)malloc(nF * sizeof(UINT32));
    if (!MycRom.TriggerID)
    {
        cprintf("Unable to allocate memory for trigger IDs");
        Free_cRom();
        return false;
    }
    memset(MycRom.TriggerID, 0xFF, nF * sizeof(UINT32));
    MycRP.FrameDuration = (UINT32*)malloc(nF * sizeof(UINT32));
    if (!MycRP.FrameDuration)
    {
        cprintf("Unable to allocate memory for frame duration");
        Free_cRom();
        return false;
    }
    MycRom.SpriteDescriptions = NULL;
    MycRom.SpriteDetDwords = NULL;
    MycRom.SpriteDetDwordPos = NULL;
    MycRom.SpriteDetAreas = NULL;
    MycRom.ColorRotations = (UINT8*)malloc(nF * 3 * MAX_COLOR_ROTATION);
    if (!MycRom.ColorRotations)
    {
        cprintf("Unable to allocate memory for color rotations");
        Free_cRom();
        return false;
    }
    memset(MycRom.ColorRotations, 255, nF * 3 * MAX_COLOR_ROTATION);
    MycRP.nSections = 0;
    MycRom.nFrames = 0;
    for (unsigned int tk = 0; tk < nFrames; tk++)
    {
        if (pFrames[tk].active == TRUE)
        {
            MycRP.FrameDuration[MycRom.nFrames] = pFrames[tk].timecode;
            char* psFr = pFrames[tk].ptr;
            UINT8* pdoFr = &MycRP.oFrames[MycRom.fWidth * MycRom.fHeight * MycRom.nFrames];
            UINT8* pdcFr = &MycRom.cFrames[MycRom.fWidth * MycRom.fHeight * MycRom.nFrames];
/*            if (tk < nFrames - 1)
            {
                UINT32 time1 = pFrames[tk].timecode;
                UINT32 time2 = pFrames[tk + 1].timecode;
                if (time2 < time1) MycRP.FrameDuration[MycRom.nFrames] = 0;
                else if (time2 - time1 > 30000) MycRP.FrameDuration[MycRom.nFrames] = 0;
                else MycRP.FrameDuration[MycRom.nFrames] = time2 - time1;
                if (filter_time && (filter_length > MycRP.FrameDuration[MycRom.nFrames])) pFrames[tk].active = FALSE;
            }
            else MycRP.FrameDuration[MycRom.nFrames] = 0;*/
            Init_cFrame_Palette(&MycRom.cPal[sizepalette * MycRom.nFrames]);
            for (unsigned int tj = 0; tj < MycRom.fHeight* MycRom.fWidth; tj++)
            {
                *pdcFr = *pdoFr = (UINT8)(*psFr);
                pdoFr++;
                pdcFr++;
                psFr++;
            }
            MycRom.nFrames++;
        }
    }
    return true;
}

bool AddTXTFrames2Frame(UINT nFrames, sFrames* pFrames)
{
    // Add frames from a new txt file to the already 
    // count remaining original frames
    unsigned int nF = 0;
    for (unsigned int ti = 0; ti < nFrames; ti++)
    {
        if (pFrames[ti].active == TRUE) nF++;
    }
    if (nF == 0) return true;
    // reallocating cROM Frame space
    MycRP.oFrames = (UINT8*)realloc(MycRP.oFrames, (nF + MycRom.nFrames) * sizeof(UINT8) * MycRom.fWidth * MycRom.fHeight);
    if (!MycRP.oFrames)
    {
        cprintf("Unable to reallocate memory for original frames");
        return false;
    }
    MycRom.HashCode = (UINT*)realloc(MycRom.HashCode, (nF + MycRom.nFrames) * sizeof(UINT));
    if (!MycRom.HashCode)
    {
        Free_cRom();
        cprintf("Unable to reallocate memory for hashcodes");
        return false;
    }
    MycRom.ShapeCompMode = (UINT8*)realloc(MycRom.ShapeCompMode, nF + MycRom.nFrames);
    if (!MycRom.ShapeCompMode)
    {
        Free_cRom();
        cprintf("Unable to allocate memory for shape mode");
        return false;
    }
    memset(&MycRom.ShapeCompMode[MycRom.nFrames], FALSE, nF);
    MycRom.CompMaskID = (UINT8*)realloc(MycRom.CompMaskID, (nF + MycRom.nFrames) * sizeof(UINT8));
    if (!MycRom.CompMaskID)
    {
        Free_cRom();
        cprintf("Unable to reallocate memory for comparison masks");
        return false;
    }
    memset(&MycRom.CompMaskID[MycRom.nFrames], 255, nF * sizeof(UINT8));
    MycRom.MovRctID = (UINT8*)realloc(MycRom.MovRctID, (nF + MycRom.nFrames) * sizeof(UINT8));
    if (!MycRom.MovRctID)
    {
        Free_cRom();
        cprintf("Unable to reallocate memory for moving mask IDs");
        return false;
    }
    memset(&MycRom.MovRctID[MycRom.nFrames], 255, nF * sizeof(UINT8));
    // no need to reallocate CompMasks as the maximum is allocated from start
    size_t sizepalette = MycRom.ncColors * 3;
    MycRom.cPal = (UINT8*)realloc(MycRom.cPal, (nF + MycRom.nFrames) * sizepalette);
    if (!MycRom.cPal)
    {
        Free_cRom();
        cprintf("Unable to reallocate memory for colorized palettes");
        return false;
    }
    MycRom.cFrames = (UINT8*)realloc(MycRom.cFrames, (nF + MycRom.nFrames) * (size_t)MycRom.fWidth * (size_t)MycRom.fHeight);
    if (!MycRom.cFrames)
    {
        cprintf("Unable to reallocate memory for colorized frames");
        Free_cRom();
        return false;
    }
    MycRom.DynaMasks = (UINT8*)realloc(MycRom.DynaMasks, (nF + MycRom.nFrames) * (size_t)MycRom.fWidth * (size_t)MycRom.fHeight);
    if (!MycRom.DynaMasks)
    {
        cprintf("Unable to reallocate memory for dynamic masks");
        Free_cRom();
        return false;
    }
    memset(&MycRom.DynaMasks[MycRom.nFrames * MycRom.fWidth * MycRom.fHeight], 255, nF * MycRom.fWidth * MycRom.fHeight);
    MycRom.Dyna4Cols = (UINT8*)realloc(MycRom.Dyna4Cols, (nF + MycRom.nFrames) * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors);
    if (!MycRom.Dyna4Cols)
    {
        cprintf("Unable to reallocate memory for dynamic color sets");
        Free_cRom();
        return false;
    }
    for (UINT tj = MycRom.nFrames; tj < MycRom.nFrames + nF; tj++)
    {
        for (UINT ti = 0; ti < MAX_DYNA_SETS_PER_FRAME; ti++)
        {
            for (UINT tk = 0; tk < MycRom.noColors; tk++) MycRom.Dyna4Cols[tj * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors + ti * MycRom.noColors + tk] = (UINT8)(ti * MycRom.noColors + tk);
        }
    }
    MycRom.FrameSprites = (UINT8*)realloc(MycRom.FrameSprites, (nF + MycRom.nFrames) * MAX_SPRITES_PER_FRAME);
    if (!MycRom.FrameSprites)
    {
        cprintf("Unable to reallocate memory for sprite IDs");
        Free_cRom();
        return false;
    }
    memset(&MycRom.FrameSprites[MycRom.nFrames * MAX_SPRITES_PER_FRAME], 255, nF * MAX_SPRITES_PER_FRAME);
    MycRom.TriggerID = (UINT32*)realloc(MycRom.TriggerID, (nF + MycRom.nFrames) * sizeof(UINT32));
    if (!MycRom.TriggerID)
    {
        cprintf("Unable to reallocate memory for trigger IDs");
        Free_cRom();
        return false;
    }
    memset(&MycRom.TriggerID[MycRom.nFrames], 255, nF * sizeof(UINT32));
    MycRom.ColorRotations = (UINT8*)realloc(MycRom.ColorRotations, (nF + MycRom.nFrames) * 3 * MAX_COLOR_ROTATION);
    if (!MycRom.ColorRotations)
    {
        cprintf("Unable to reallocate memory for color rotations");
        Free_cRom();
        return false;
    }
    memset(&MycRom.ColorRotations[MycRom.nFrames*3*MAX_COLOR_ROTATION], 255, nF * 3 * MAX_COLOR_ROTATION);
    MycRP.FrameDuration = (UINT32*)realloc(MycRP.FrameDuration, (nF + MycRom.nFrames) * sizeof(UINT32));
    if (!MycRP.FrameDuration)
    {
        cprintf("Unable to reallocate memory for frame duration");
        Free_cRom();
        return false;
    }
    for (unsigned int tk = 0; tk < nFrames; tk++)
    {
        if (pFrames[tk].active == TRUE)
        {
            MycRP.FrameDuration[MycRom.nFrames]= pFrames[tk].timecode;
            UINT8* psFr = (UINT8*)pFrames[tk].ptr;
            UINT8* pdoFr = &MycRP.oFrames[MycRom.fWidth * MycRom.fHeight * MycRom.nFrames];
            UINT8* pdcFr = &MycRom.cFrames[MycRom.fWidth * MycRom.fHeight * MycRom.nFrames];
/*            if (tk < nFrames - 1)
            {
                UINT32 time1 = pFrames[tk].timecode;
                UINT32 time2 = pFrames[tk + 1].timecode;
                if (time2 < time1) MycRP.FrameDuration[MycRom.nFrames] = 0;
                else if (time2 - time1 > 30000) MycRP.FrameDuration[MycRom.nFrames] = 0;
                else MycRP.FrameDuration[MycRom.nFrames] = time2 - time1;
            }
            else MycRP.FrameDuration[MycRom.nFrames] = 0;*/
            Init_cFrame_Palette(&MycRom.cPal[sizepalette * MycRom.nFrames]);
            for (unsigned int tj = 0; tj < MycRom.fHeight * MycRom.fWidth; tj++)
            {
                *pdcFr = *pdoFr = *psFr;
                pdoFr++;
                pdcFr++;
                psFr++;
            }
            MycRom.nFrames++;
        }
    }
    return true;
}

void Load_TXT_File(void)
{
    char acDir[260];
    GetCurrentDirectoryA(260, acDir);

    char* TXTF_buffer = NULL; // Loaded 
    size_t TXTF_buffer_len = 0;
    unsigned int nFrames = 0; // number of frames inside the txt file
    sFrames* pFrames = NULL; // txt file frame description

    // Load a txt file as a base for a new project
    OPENFILENAMEA ofn;
    char szFile[260]={0};

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrTitle = "Choose the initial TXT file";
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Text (.txt)\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrInitialDir = DumpDir;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        strcpy_s(MycRP.SaveDir, 260, ofn.lpstrFile);
        int i = (int)strlen(MycRP.SaveDir) - 1;
        while ((i > 0) && (MycRP.SaveDir[i] != '\\')) i--;
        MycRP.SaveDir[i + 1] = 0;
        if (isLoadedProject)
        {
            if (MessageBox(hWnd, L"Confirm you want to close the current project and load a new one", L"Caution", MB_YESNO) == IDYES)
            {
                Free_Project();
            }
            else
            {
                SetCurrentDirectoryA(acDir);
                return;
            }
        }
        FILE* pfile;
        if (fopen_s(&pfile, ofn.lpstrFile, "rb") != 0)
        {
            cprintf("Unable to open the file %s", ofn.lpstrFile);
            SetCurrentDirectoryA(acDir);
            return;
        }
        if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_FILTERS), hWnd, Filter_Proc, (LPARAM)1) == -1)
        {
            fclose(pfile);
            return;
        }
        fseek(pfile, 0, SEEK_END);
        TXTF_buffer_len = (size_t)ftell(pfile);
        rewind(pfile);
        TXTF_buffer = (char*)malloc(TXTF_buffer_len + 1);
        if (!TXTF_buffer)
        {
            TXTF_buffer_len = 0;
            cprintf("Unable to get the memory buffer for the TXT file");
            fclose(pfile);
            SetCurrentDirectoryA(acDir);
            return;
        }
        size_t nread = fread_s(TXTF_buffer, TXTF_buffer_len + 1, 1, TXTF_buffer_len, pfile);
        TXTF_buffer[TXTF_buffer_len] = 0;
        fclose(pfile);
        if (!Parse_TXT(ofn.lpstrFile,TXTF_buffer, TXTF_buffer_len, &pFrames, &nFrames))
        {
            free(TXTF_buffer);
            SetCurrentDirectoryA(acDir);
            return;
        }
        char tpath[MAX_PATH];
        strcpy_s(tpath, MAX_PATH, ofn.lpstrFile);
        unsigned int ti = 0;
        size_t tj = strlen(tpath) - 1;
        while ((tpath[tj] != '\\') && (tpath[tj] != ':') && (tj > 0)) tj--;
        if (tj > 0)
        {
            tj++;
            while (tpath[tj] != '.')
            {
                MycRom.name[ti] = tpath[tj];
                ti++;
                tj++;
            }
        }
        MycRom.name[ti] = 0;
        strcpy_s(MycRP.name, 64, MycRom.name);
        CompareFrames(nFrames, pFrames);
        CopyTXTFrames2Frame(nFrames, pFrames);
        free(TXTF_buffer);
        free(pFrames);
        acFrame = 0;
        InitColorRotation();
        nSelFrames = 1;
        SelFrames[0] = 0;
        PreFrameInStrip = 0;
        isLoadedProject = true;
    }
    SetCurrentDirectoryA(acDir);
}

LRESULT CALLBACK Filter_Proc(HWND hwDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
    {
        SendMessage(GetDlgItem(hwDlg, IDC_DELTIME), BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(GetDlgItem(hwDlg, IDC_DELMASK), BM_SETCHECK, BST_UNCHECKED, 0);
        SendMessage(GetDlgItem(hwDlg, IDC_DELCOL), BM_SETCHECK, BST_UNCHECKED, 0);
        EnableWindow(GetDlgItem(hwDlg, IDC_TIMELEN), FALSE);
        EnableWindow(GetDlgItem(hwDlg, IDC_NCOL), FALSE);
        if (lParam==0) EnableWindow(GetDlgItem(hwDlg, IDC_DELMASK), TRUE); else EnableWindow(GetDlgItem(hwDlg, IDC_DELMASK), FALSE);
    }
    case WM_COMMAND:
    {
        switch (wParam)
        {
        case IDC_DELTIME:
        {
            if (SendMessage(GetDlgItem(hwDlg, IDC_DELTIME), BM_GETCHECK, 0, 0) == BST_UNCHECKED) EnableWindow(GetDlgItem(hwDlg, IDC_TIMELEN), FALSE);
            else EnableWindow(GetDlgItem(hwDlg, IDC_TIMELEN), TRUE);
            return TRUE;
        }
        case IDC_DELCOL:
        {
            if (SendMessage(GetDlgItem(hwDlg, IDC_DELCOL), BM_GETCHECK, 0, 0) == BST_UNCHECKED) EnableWindow(GetDlgItem(hwDlg, IDC_NCOL), FALSE);
            else EnableWindow(GetDlgItem(hwDlg, IDC_NCOL), TRUE);
            return TRUE;
        }
        case IDOK:
        {
            bool istimemod = false;
            bool iscolmod = false;
            if (SendMessage(GetDlgItem(hwDlg, IDC_DELTIME), BM_GETCHECK, 0, 0) == BST_CHECKED)
            {
                char tbuf[256];
                filter_time = true;
                GetWindowTextA(GetDlgItem(hwDlg, IDC_TIMELEN), tbuf, 256);
                filter_length = atoi(tbuf);
                if (filter_length < 5)
                {
                    filter_length = 5;
                    _itoa_s(filter_length, tbuf, 256, 10);
                    SetWindowTextA(GetDlgItem(hwDlg, IDC_TIMELEN), tbuf);
                    istimemod = true;
                }
                else if (filter_length > 3000)
                {
                    filter_length = 3000;
                    _itoa_s(filter_length, tbuf, 256, 10);
                    SetWindowTextA(GetDlgItem(hwDlg, IDC_TIMELEN), tbuf);
                    istimemod = true;
                }
            }
            else filter_time = false;
            if (SendMessage(GetDlgItem(hwDlg, IDC_DELCOL), BM_GETCHECK, 0, 0) == BST_CHECKED)
            {
                char tbuf[256];
                filter_color = true;
                GetWindowTextA(GetDlgItem(hwDlg, IDC_NCOL), tbuf, 256);
                filter_ncolor = atoi(tbuf);
                if (filter_ncolor < 1)
                {
                    filter_ncolor = 1;
                    _itoa_s(filter_ncolor, tbuf, 256, 10);
                    SetWindowTextA(GetDlgItem(hwDlg, IDC_NCOL), tbuf);
                    iscolmod = true;
                }
                else if (filter_ncolor > 16)
                {
                    filter_ncolor = 16;
                    _itoa_s(filter_ncolor, tbuf, 256, 10);
                    SetWindowTextA(GetDlgItem(hwDlg, IDC_NCOL), tbuf);
                    iscolmod = true;
                }
            }
            else filter_color = false;
            if (SendMessage(GetDlgItem(hwDlg, IDC_DELMASK),BM_GETCHECK,0,0) == BST_CHECKED) filter_allmask = true; else filter_allmask = false;
            if (istimemod && iscolmod)
            {
                MessageBoxA(hWnd, "The time length and number of colors values have been automatically changed, check them and confirm again", "Confirm", MB_OK);
                return TRUE;
            }
            else if (istimemod)
            {
                MessageBoxA(hWnd, "The time length value has been automatically changed, check it and confirm again", "Confirm", MB_OK);
                return TRUE;
            }
            else if (iscolmod)
            {
                MessageBoxA(hWnd, "The number of colors value has been automatically changed, check it and confirm again", "Confirm", MB_OK);
                return TRUE;
            }
            EndDialog(hwDlg, 0);
            return TRUE;
        }
        case IDCANCEL:
        {
            EndDialog(hwDlg, -1);
            return TRUE;
        }
        }
    }
    }
    return FALSE;
}

void Add_TXT_File(void)
{
    char acDir[260];
    GetCurrentDirectoryA(260, acDir);

    char* TXTF_buffer = NULL; // Loaded 
    size_t TXTF_buffer_len = 0;
    unsigned int nFrames = 0; // number of frames inside the txt file
    sFrames* pFrames = NULL; // txt file frame description

    // Add frames to the project with a new txt file
    OPENFILENAMEA ofn;
    char szFile[260];

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lpstrTitle = "Choose the additional TXT file";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Text (.txt)\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = MycRP.SaveDir;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn) == TRUE)
    {
        FILE* pfile;
        if (fopen_s(&pfile, ofn.lpstrFile, "rb") != 0)
        {
            cprintf("Unable to open the file %s", ofn.lpstrFile);
            SetCurrentDirectoryA(acDir);
            return;
        }
        if (DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_FILTERS), hWnd, Filter_Proc,(LPARAM)0) == -1)
        {
            fclose(pfile);
            return;
        }
        fseek(pfile, 0, SEEK_END);
        TXTF_buffer_len = (size_t)ftell(pfile);
        rewind(pfile);
        TXTF_buffer = (char*)malloc(TXTF_buffer_len + 1);
        if (!TXTF_buffer)
        {
            TXTF_buffer_len = 0;
            cprintf("Unable to get the memory buffer for the TXT file");
            fclose(pfile);
            SetCurrentDirectoryA(acDir);
            return;
        }
        size_t nread = fread_s(TXTF_buffer, TXTF_buffer_len + 1, 1, TXTF_buffer_len, pfile);
        TXTF_buffer[TXTF_buffer_len] = 0;
        fclose(pfile);
        if (!Parse_TXT(ofn.lpstrFile,TXTF_buffer, TXTF_buffer_len, &pFrames, &nFrames))
        {
            free(TXTF_buffer);
            SetCurrentDirectoryA(acDir);
            return;
        }
        CompareAdditionalFrames(nFrames, pFrames);
        AddTXTFrames2Frame(nFrames, pFrames);
        free(TXTF_buffer);
        free(pFrames);
    }
    SetCurrentDirectoryA(acDir);
}

#pragma endregion Txt_File_Operations

#pragma region Window_Procs

LRESULT CALLBACK Wait_Proc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    case WM_INITDIALOG:
        return TRUE;
    }

    return FALSE;
}

void SortSections(void)
{
    unsigned short sortlist[MAX_SECTIONS];
    for (UINT ti = 0; ti < MycRP.nSections; ti++) sortlist[ti] = ti;
    for (UINT ti = 0; ti < MycRP.nSections; ti++)
    {
        for (UINT tj = ti + 1; tj < MycRP.nSections; tj++)
            if (_stricmp(&MycRP.Section_Names[sortlist[tj] * SIZE_SECTION_NAMES], &MycRP.Section_Names[sortlist[ti] * SIZE_SECTION_NAMES]) < 0)  //Edit.
            {
                unsigned short tamp = sortlist[tj];
                sortlist[tj] = sortlist[ti];
                sortlist[ti] = tamp;
            }
    }
    UINT32		Section_Firsts[MAX_SECTIONS]; // first frame of each section
    char		Section_Names[MAX_SECTIONS * SIZE_SECTION_NAMES]; // Names of the sections
    for (UINT ti = 0; ti < MycRP.nSections; ti++)
    {
        Section_Firsts[ti] = MycRP.Section_Firsts[sortlist[ti]];
        strcpy_s(&Section_Names[ti * SIZE_SECTION_NAMES], SIZE_SECTION_NAMES, &MycRP.Section_Names[sortlist[ti] * SIZE_SECTION_NAMES]);
    }
    memcpy(MycRP.Section_Firsts, Section_Firsts, sizeof(UINT32) * MAX_SECTIONS);
    memcpy(MycRP.Section_Names, Section_Names, MAX_SECTIONS * SIZE_SECTION_NAMES);
}

void MoveSection(int nosec, int decalage)
{
    if (nosec == LB_ERR) return;
/*	UINT32		Section_Firsts[MAX_SECTIONS]; // first frame of each section
	char		Section_Names[MAX_SECTIONS * SIZE_SECTION_NAMES]; // Names of the sections
*/
    UINT32 sfirst;
    char sName[SIZE_SECTION_NAMES];
    sfirst = MycRP.Section_Firsts[nosec + decalage];
    MycRP.Section_Firsts[nosec + decalage] = MycRP.Section_Firsts[nosec];
    MycRP.Section_Firsts[nosec] = sfirst;
    strcpy_s(sName, SIZE_SECTION_NAMES, &MycRP.Section_Names[(nosec + decalage) * SIZE_SECTION_NAMES]);
    strcpy_s(&MycRP.Section_Names[(nosec + decalage) * SIZE_SECTION_NAMES], SIZE_SECTION_NAMES, &MycRP.Section_Names[nosec * SIZE_SECTION_NAMES]);
    strcpy_s(&MycRP.Section_Names[nosec * SIZE_SECTION_NAMES], SIZE_SECTION_NAMES, sName);
}

HWND hListBox, hwndButtonU, hwndButtonD, hwndButtonAlpha;
LRESULT CALLBACK MovSecProc(HWND hWin, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        // Adding a ListBox.
        hListBox = CreateWindowExW(WS_EX_CLIENTEDGE, L"LISTBOX", NULL,WS_CHILD | WS_VISIBLE | WS_VSCROLL,25, 25, 200, 550, hWin, NULL, hInst, NULL);
        if (!hListBox)
        {
            AffLastError((char*)"Create ListBox Window");
            return FALSE;
        }
        ShowWindow(hListBox, TRUE);
        hwndButtonU = CreateWindow(L"BUTTON", L"UP", WS_TABSTOP | WS_VISIBLE | WS_CHILD, 230, 205, 50, 50, hWin, NULL, hInst, NULL);      // Pointer not needed.
        if (!hwndButtonU)
        {
            AffLastError((char*)"Create Button Up Window");
            return FALSE;
        }
        ShowWindow(hwndButtonU, TRUE);
        hwndButtonAlpha = CreateWindow(L"BUTTON", L"ALPHA", WS_TABSTOP | WS_VISIBLE | WS_CHILD, 230, 275, 50, 50, hWin, NULL, hInst, NULL);      // Pointer not needed.
        if (!hwndButtonAlpha)
        {
            AffLastError((char*)"Create Button Alpha Window");
            return FALSE;
        }
        ShowWindow(hwndButtonU, TRUE);
        hwndButtonD = CreateWindow(L"BUTTON", L"DOWN", WS_TABSTOP | WS_VISIBLE | WS_CHILD, 230, 345, 50, 50, hWin, NULL, hInst, NULL);      // Pointer not needed.
        if (!hwndButtonD)
        {
            AffLastError((char*)"Create Button Down Window");
            return FALSE;
        }
        ShowWindow(hwndButtonD, TRUE);
        for (UINT ti = 0; ti < MycRP.nSections; ti++)
        {
            SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)&MycRP.Section_Names[ti * SIZE_SECTION_NAMES]);
        }
        return TRUE;
    }
    case WM_MOUSEMOVE:
    {
        TRACKMOUSEEVENT me{};
        me.cbSize = sizeof(TRACKMOUSEEVENT);
        me.dwFlags = TME_LEAVE;
        me.hwndTrack = hMovSec;
        me.dwHoverTime = HOVER_DEFAULT;
        TrackMouseEvent(&me);
    }
    case WM_MOUSELEAVE:
    {
        RECT rc;
        POINT pt;
        GetClientRect(hWin, &rc);
        GetCursorPos(&pt);
        ScreenToClient(hWin, &pt);
        if ((pt.x < 0) || (pt.x >= rc.right) || (pt.y < 0) || (pt.y >= rc.bottom))
        {
            UpdateSectionList();
            UpdateFSneeded = true;
            DestroyWindow(hMovSec);
            hMovSec = NULL;
        }
        return TRUE;
    }
    case WM_COMMAND:
    {
        int acpos=(int)SendMessage(hListBox,LB_GETCURSEL,0,0);
        if (((HWND)lParam == hwndButtonU) && (acpos > 0))
        {
            if (acpos == LB_ERR) return TRUE;
            MoveSection(acpos, -1);
            SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
            for (UINT ti = 0; ti < MycRP.nSections; ti++)
            {
                SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)&MycRP.Section_Names[ti * SIZE_SECTION_NAMES]);
            }
            SendMessage(hListBox, LB_SETCURSEL, acpos - 1, 0);
        }
        else if (((HWND)lParam == hwndButtonD) && (acpos < (int)MycRP.nSections - 1))
        {
            if (acpos == LB_ERR) return TRUE;
            MoveSection(acpos, +1);
            SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
            for (UINT ti = 0; ti < MycRP.nSections; ti++)
            {
                SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)&MycRP.Section_Names[ti * SIZE_SECTION_NAMES]);
                SendMessage(hListBox, LB_SETCURSEL, acpos + 1, 0);
            }
        }
        else if ((HWND)lParam == hwndButtonAlpha)
        {
            if (MessageBoxA(hWnd, "Do you really want to reorder section according the names?", "Confirm?", MB_YESNO) == IDYES)
            {
                SortSections();
                SendMessage(hListBox, LB_RESETCONTENT, 0, 0);
                for (UINT ti = 0; ti < MycRP.nSections; ti++)
                {
                    SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)&MycRP.Section_Names[ti * SIZE_SECTION_NAMES]);
                    SendMessage(hListBox, LB_SETCURSEL, acpos + 1, 0);
                }
            }
            UpdateSectionList();
            UpdateFSneeded = true;
        }
        return TRUE;
    }
    default:
        return DefWindowProc(hWin, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK PalProc(HWND hWin, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_MOUSEMOVE:
    {
        TRACKMOUSEEVENT me{};
        me.cbSize = sizeof(TRACKMOUSEEVENT);
        me.dwFlags = TME_HOVER | TME_LEAVE;
        me.hwndTrack = hWin;
        me.dwHoverTime = HOVER_DEFAULT;
        TrackMouseEvent(&me);
        if (hWin == hPal2) break;
        if (hWin == hPal)
        {
            if (Start_Gradient)
            {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWin, &pt);
                if ((pt.x < MARGIN_PALETTE) || (pt.x >= 160 + MARGIN_PALETTE) || (pt.y < MARGIN_PALETTE) || (pt.y >= 160 + MARGIN_PALETTE)) break;
                Fin_Gradient_Color = ((UINT8)(pt.x - MARGIN_PALETTE) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE) / 20);
            }
            else if (Start_Gradient2)
            {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWin, &pt);
                if ((pt.x < MARGIN_PALETTE) || (pt.x >= 160 + MARGIN_PALETTE) || (pt.y < MARGIN_PALETTE) || (pt.y >= 160 + MARGIN_PALETTE)) break;
                Draw_Grad_Fin = ((UINT8)(pt.x - MARGIN_PALETTE) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE) / 20);
            }
        }
        else
        {
            if (Start_Gradient)
            {
                POINT pt;
                GetCursorPos(&pt);
                ScreenToClient(hWin, &pt);
                if ((pt.x < MARGIN_PALETTE) || (pt.x >= 160 + MARGIN_PALETTE) || (pt.y < MARGIN_PALETTE) || (pt.y >= 160 + MARGIN_PALETTE)) break;
                Fin_Gradient_Color = ((UINT8)(pt.x - MARGIN_PALETTE) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE) / 20);
                
            }
        }
        break;
    }
    case WM_MOUSELEAVE:
    {
        Start_Gradient = false;
        Start_Gradient2 = false;
        Start_Col_Exchange = false;
        DestroyWindow(hWin);
        if (hWin==hPal) InvalidateRect(GetDlgItem(hwTB, IDC_GRADMODEB), NULL, FALSE);
        if (hWin == hPal3)
        {
            InvalidateRect(GetDlgItem(hColSet, IDC_COLORS), NULL, TRUE);
            InvalidateRect(hwTB, NULL, TRUE);
        }
        hPal = hPal2 = hPal3 = NULL;
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = GetDC(hWin);
        RECT rc;
        HBRUSH hbr;
        UINT8 acCol = acEditColors[noColMod];
        if ((hWin == hPal) && (noColMod >= 16))
        {
            acCol = MycRom.Dyna4Cols[(acFrame * MAX_DYNA_SETS_PER_FRAME + acDynaSet) * MycRom.noColors + +noColMod - 16];
        }
        if (hWin == hPal2) acCol = MycRP.Sprite_Edit_Colors[noColMod];
        BeginPaint(hWin, &ps);
        for (UINT tj = 0; tj < 8; tj++)
        {
            for (UINT ti = 0; ti < 8; ti++)
            {
                rc.left = MARGIN_PALETTE + 20 * ti;
                rc.right = MARGIN_PALETTE + 20 * (ti + 1) - 1;
                rc.top = MARGIN_PALETTE + 20 * tj;
                rc.bottom = MARGIN_PALETTE + 20 * (tj + 1) - 1;
                if ((hWin == hPal) || (hWin == hPal3)) hbr = CreateSolidBrush(RGB(MycRom.cPal[3 * MycRom.ncColors * acFrame + (tj * 8 + ti) * 3], MycRom.cPal[3 * MycRom.ncColors * acFrame + (tj * 8 + ti) * 3 + 1], MycRom.cPal[3 * MycRom.ncColors * acFrame + (tj * 8 + ti) * 3 + 2]));
                else hbr= CreateSolidBrush(RGB(MycRom.cPal[3 * MycRom.ncColors * MycRP.Sprite_Col_From_Frame[acSprite] + (tj * 8 + ti) * 3], MycRom.cPal[3 * MycRom.ncColors * MycRP.Sprite_Col_From_Frame[acSprite] + (tj * 8 + ti) * 3 + 1], MycRom.cPal[3 * MycRom.ncColors * MycRP.Sprite_Col_From_Frame[acSprite] + (tj * 8 + ti) * 3 + 2]));
                FillRect(hdc, &rc, hbr);
                if ((hWin != hPal3) && ((hWin == hPal2) || ((!Start_Gradient) && (!Start_Gradient2) && (!Start_Col_Exchange))))
                {
                    if (tj * 8 + ti == acCol)
                    {
                        DeleteObject(hbr);
                        hbr = CreateSolidBrush(RGB(SelFrameColor, SelFrameColor, SelFrameColor));
                        FrameRect(hdc, &rc, hbr);
                    }
                }
                else if (Start_Gradient)
                {
                    if ((tj * 8 + ti >= min(Ini_Gradient_Color, Fin_Gradient_Color)) && (tj * 8 + ti <= max(Ini_Gradient_Color, Fin_Gradient_Color)))
                    {
                        DeleteObject(hbr);
                        hbr = CreateSolidBrush(RGB(0, SelFrameColor, SelFrameColor));
                        FrameRect(hdc, &rc, hbr);
                    }
                }
                else if (Start_Gradient2)
                {
                    if ((tj * 8 + ti >= min(Draw_Grad_Ini, Draw_Grad_Fin)) && (tj * 8 + ti <= max(Draw_Grad_Ini, Draw_Grad_Fin)))
                    {
                        DeleteObject(hbr);
                        hbr = CreateSolidBrush(RGB(SelFrameColor, 0, SelFrameColor));
                        FrameRect(hdc, &rc, hbr);
                    }
                }
                else if (Start_Col_Exchange)
                {
                    if (tj * 8 + ti == Pre_Col_Exchange)
                    {
                        DeleteObject(hbr);
                        hbr = CreateSolidBrush(RGB(SelFrameColor, 0, SelFrameColor));
                        FrameRect(hdc, &rc, hbr);
                    }
                }
                UINT quelfr = acFrame;
                if (hWin == hPal2) quelfr = MycRP.Sprite_Col_From_Frame[acSprite];
                //else if (hWin == hPal3) quelfr = 255;
                if (!Is_Used_Color(quelfr, tj * 8 + ti))
                {
                    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(SelFrameColor, SelFrameColor, SelFrameColor));
                    SelectObject(hdc, hPen);
                    MoveToEx(hdc, rc.left, rc.top, NULL);
                    LineTo(hdc, rc.right, rc.bottom);
                    DeleteObject(hPen);
                }
                DeleteObject(hbr);
            }
        }
        EndPaint(hWin, &ps);
        break;
    }
    case WM_LBUTTONDOWN:
    {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hWin, &pt);
        if ((pt.x < MARGIN_PALETTE) || (pt.x >= 160 + MARGIN_PALETTE) || (pt.y < MARGIN_PALETTE) || (pt.y >= 160 + MARGIN_PALETTE)) break;
        if (hWin == hPal2)
        {
            SaveAction(true, SA_SPRITECOLOR);
            MycRP.Sprite_Edit_Colors[noColMod + acSprite * 16] = ((UINT8)(pt.x - MARGIN_PALETTE) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE) / 20);
            InvalidateRect(hWin, NULL, TRUE);
            InvalidateRect(hwTB2, NULL, TRUE);
        }
        else if (Start_Col_Exchange)
        {
            UINT8 Der_Col_Exchange = ((UINT8)(pt.x - MARGIN_PALETTE) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE) / 20);
            for (int tj = 0; tj < (int)nSelFrames; tj++)
            {
                for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
                {
                    if (MycRom.cFrames[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti] == Pre_Col_Exchange) MycRom.cFrames[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti] = Der_Col_Exchange;
                    else if (MycRom.cFrames[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti] == Der_Col_Exchange) MycRom.cFrames[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti] = Pre_Col_Exchange;
                }
                UINT8 Rexc = MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + Pre_Col_Exchange * 3];
                UINT8 Vexc = MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + Pre_Col_Exchange * 3 + 1];
                UINT8 Bexc = MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + Pre_Col_Exchange * 3 + 2];
                MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + Pre_Col_Exchange * 3] = MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + Der_Col_Exchange * 3];
                MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + Pre_Col_Exchange * 3 + 1] = MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + Der_Col_Exchange * 3 + 1];
                MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + Pre_Col_Exchange * 3 + 2] = MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + Der_Col_Exchange * 3 + 2];
                MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + Der_Col_Exchange * 3] = Rexc;
                MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + Der_Col_Exchange * 3 + 1] = Vexc;
                MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + Der_Col_Exchange * 3 + 2] = Bexc;
            }
            Start_Col_Exchange = false;
        }
        else if ((hWin == hPal) && (!(GetKeyState(VK_SHIFT) & 0x8000)) && (!(GetKeyState(VK_CONTROL) & 0x8000)) && (!(GetKeyState(VK_MENU) & 0x8000)) && (!Start_Col_Exchange)) // just select a color
        {
            if (noColMod < 16) {
                SaveAction(true, SA_EDITCOLOR);
                acEditColors[noColMod] = ((UINT8)(pt.x - MARGIN_PALETTE) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE) / 20);
            }
            else
            {
                SaveAction(true, SA_DYNACOLOR);
                for (UINT tj = 0; tj < nSelFrames; tj++)
                {
                    MycRom.Dyna4Cols[SelFrames[tj] * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors + acDynaSet * MycRom.noColors + (noColMod - 16)] = ((UINT8)(pt.x - MARGIN_PALETTE) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE) / 20);
                }
            }
            DestroyWindow(hWin);
            int colid;
            if (noColMod < 16) colid = IDC_COL1 + noColMod;
            else colid = IDC_DYNACOL1 + noColMod - 16;
            InvalidateRect(GetDlgItem(hwTB, colid), NULL, TRUE);
        }
        else if (GetKeyState(VK_SHIFT) & 0x8000) // create a color gradient or select a color sequence for rotation
        {
            Start_Gradient = true;
            Ini_Gradient_Color = ((UINT8)(pt.x - MARGIN_PALETTE) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE) / 20);
            Fin_Gradient_Color = Ini_Gradient_Color;
        }
        else if ((hWin == hPal) && (GetKeyState(VK_CONTROL) & 0x8000)) // create a gradient for filling
        {
            Start_Gradient2 = true;
            Draw_Grad_Ini = ((UINT8)(pt.x - MARGIN_PALETTE) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE) / 20);
            Draw_Grad_Fin = Draw_Grad_Ini;
        }
        else if ((hWin == hPal) && (GetKeyState(VK_MENU) & 0x8000)) // invert 2 colors
        {
            if (!Start_Col_Exchange)
            {
                Start_Col_Exchange = true;
                Pre_Col_Exchange = ((UINT8)(pt.x - MARGIN_PALETTE) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE) / 20);
            }
        }
        break;
    }
    case WM_LBUTTONUP:
    {
        if (hWin == hPal2) break;
        if (Start_Gradient)
        {
            if (hWin == hPal)
            {
                SaveAction(true, SA_PALETTE);
                if (abs(Ini_Gradient_Color - Fin_Gradient_Color) < 2) break; // No gradient to calculate
                UINT8 icol = min(Ini_Gradient_Color, Fin_Gradient_Color);
                UINT8 fcol = max(Ini_Gradient_Color, Fin_Gradient_Color);
                UINT8* pcoli = &MycRom.cPal[acFrame * 3 * MycRom.ncColors + icol * 3];
                UINT8* pcolf = &MycRom.cPal[acFrame * 3 * MycRom.ncColors + fcol * 3];
                float nint = (float)(fcol - icol);
                for (UINT8 ti = icol + 1; ti < fcol; ti++)
                {
                    for (UINT tj = 0; tj < nSelFrames; tj++)
                    {
                        MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + ti * 3] = (UINT8)((INT8)pcoli[0] + (INT8)((float)(ti - icol) * ((float)pcolf[0] - (float)pcoli[0]) / nint));
                        MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + ti * 3 + 1] = (UINT8)((INT8)pcoli[1] + (INT8)((float)(ti - icol) * ((float)pcolf[1] - (float)pcoli[1]) / nint));
                        MycRom.cPal[SelFrames[tj] * 3 * MycRom.ncColors + ti * 3 + 2] = (UINT8)((INT8)pcoli[2] + (INT8)((float)(ti - icol) * ((float)pcolf[2] - (float)pcoli[2]) / nint));
                    }
                }
                InvalidateRect(hWin, NULL, TRUE);
                InvalidateRect(hwTB, NULL, TRUE);
            }
            else
            {
                SaveAction(true, SA_COLROT);
                for (UINT ti = 0; ti < nSelFrames; ti++)
                {
                    MycRom.ColorRotations[SelFrames[ti] * 3 * MAX_COLOR_ROTATION + 3 * acColRot] = min(Ini_Gradient_Color, Fin_Gradient_Color);
                    MycRom.ColorRotations[SelFrames[ti] * 3 * MAX_COLOR_ROTATION + 3 * acColRot + 1] = max(Ini_Gradient_Color, Fin_Gradient_Color) - min(Ini_Gradient_Color, Fin_Gradient_Color) + 1;
                    MycRom.ColorRotations[SelFrames[ti] * 3 * MAX_COLOR_ROTATION + 3 * acColRot + 2] = 5;
                }
                UpdateColorRotDur(hColSet);
                InitColorRotation();
                InvalidateRect(GetDlgItem(hColSet, IDC_COLORS), NULL, TRUE);
                DestroyWindow(hWin);
                hPal = hPal2 = hPal3 = NULL;
            }
            Start_Col_Exchange = false;
        }
        Start_Gradient = false;
        Start_Gradient2 = false;
//        Start_Col_Exchange = false;
        break;
    }
    case WM_RBUTTONDOWN:
    {
        if ((hWin == hPal2) || (hWin == hPal3)) break;
        UINT8 RGB[3];
        POINT pt;
        GetCursorPos(&pt);
        LONG xm = pt.x, ym = pt.y;
        ScreenToClient(hWin, &pt);
        if ((pt.x < MARGIN_PALETTE) || (pt.x >= 160 + MARGIN_PALETTE) || (pt.y < MARGIN_PALETTE) || (pt.y >= 160 + MARGIN_PALETTE)) break;
        UINT8 colclicked= ((UINT8)(pt.x - MARGIN_PALETTE) / 20) + 8 * ((UINT8)(pt.y - MARGIN_PALETTE) / 20);
        RGB[0] = MycRom.cPal[acFrame * 3 * MycRom.ncColors + colclicked * 3];
        RGB[1] = MycRom.cPal[acFrame * 3 * MycRom.ncColors + colclicked * 3 + 1];
        RGB[2] = MycRom.cPal[acFrame * 3 * MycRom.ncColors + colclicked * 3 + 2];
        if (ColorPicker(RGB,xm,ym))
        {
            for (UINT ti = 0; ti < nSelFrames; ti++)
            {
                MycRom.cPal[SelFrames[ti] * 3 * MycRom.ncColors + colclicked * 3] = RGB[0];
                MycRom.cPal[SelFrames[ti] * 3 * MycRom.ncColors + colclicked * 3 + 1] = RGB[1];
                MycRom.cPal[SelFrames[ti] * 3 * MycRom.ncColors + colclicked * 3 + 2] = RGB[2];
            }
            InvalidateRect(hPal, NULL, FALSE);
            for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
            for (UINT ti = IDC_DYNACOL1; ti <= IDC_DYNACOL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
        }
        break;
    }
    default:
        return DefWindowProc(hWin, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK WndProc(HWND hWin, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        break;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Analyse les sélections de menu :
        switch (wmId)
        {
        case IDM_EXIT:
            DestroyWindow(hWin);
            break;
        default:
            return DefWindowProc(hWin, message, wParam, lParam);
        }
        break;
    }
    case WM_MOUSEMOVE:
        if (Paste_Mode)
        {
            POINT tpt;
            GetCursorPos(&tpt);
            paste_offsetx = -(paste_centerx - tpt.x) / frame_zoom;
            paste_offsety = -(paste_centery - tpt.y) / frame_zoom;
            return TRUE;
        }
        break;
    case WM_MOUSEWHEEL:
    {
        if (MycRom.name[0] == 0) return TRUE;
        short step = (short)HIWORD(wParam) / WHEEL_DELTA;
        if (!(LOWORD(wParam) & MK_SHIFT)) PreFrameInStrip -= step;
        else PreFrameInStrip -= step * (int)NFrameToDraw;
        if (PreFrameInStrip < 0) PreFrameInStrip = 0;
        if (PreFrameInStrip >= (int)MycRom.nFrames) PreFrameInStrip = (int)MycRom.nFrames - 1;
        UpdateFSneeded = true;
        return TRUE;
    }
    case WM_SETCURSOR:
    {
        if (Paste_Mode) SetCursor(hcPaste);
        else if (Color_Pipette) SetCursor(hcColPick);
        else DefWindowProc(hWin, message, wParam, lParam);
        return TRUE;
    }
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 1152 + 16; // minimum 1152
        mmi->ptMinTrackSize.y = 546 + 59; // per 546
        return 0;
        break;
    }
    case WM_SIZE:
    {
        if (!IsIconic(hWnd))
        {
            Calc_Resize_Frame();
            UpdateFSneeded = true;
            int cxClient = LOWORD(lParam);
            int cyClient = HIWORD(lParam);
            // Resize the status bar to fit the new client area width
            SendMessage(hStatus, WM_SIZE, 0, MAKELPARAM(cxClient, cyClient));
            // Position the status bar at the bottom of the window
            RECT rcStatusBar;
            GetWindowRect(hStatus, &rcStatusBar);
            int statusBarHeight = rcStatusBar.bottom - rcStatusBar.top;
            MoveWindow(hStatus, 0, cyClient - statusBarHeight, cxClient, statusBarHeight, TRUE);
            break;
        }
    } // no "break" here as the code in WM_MOVE is common
    case WM_MOVE:
    {
        if (Paste_Mode)
        {
            int xmin = 257, xmax = -1, ymin = 65, ymax = -1;
            for (int tj = 0; tj < (int)Paste_Height; tj++)
            {
                for (int ti = 0; ti < (int)Paste_Width; ti++)
                {
                    if (Paste_Mask[ti + tj * Paste_Width] == 0) continue;
                    if (ti < xmin) xmin = ti;
                    if (ti > xmax) xmax = ti;
                    if (tj < ymin) ymin = tj;
                    if (tj > ymax) ymax = tj;
                }
            }
            int tx, ty;
            glfwGetWindowPos(glfwframe, &tx, &ty);
            paste_centerx = tx + frame_zoom * (xmax + xmin) / 2;
            paste_centery = ty + frame_zoom * (ymax + ymin) / 2;
        }
        break;
    }
    /*case WM_TIMER:
    {
        if (wParam == 0)
        {
            if (hwndTip)
            {
                DestroyWindow(hwndTip);
                hwndTip = NULL;
            }
        }
        break;
    }*/
    case WM_CLOSE:
    {
        if (MessageBoxA(hWnd, "Confirm you want to exit?", "Confirm", MB_YESNO) == IDYES)
        {
            SaveWindowPosition();
            DestroyWindow(hWnd);
        }
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        fDone = true;
        break;
    }
    default:
        return DefWindowProc(hWin, message, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK WndProc2(HWND hWin, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        if (!IsIconic(hWin))
        {
            Calc_Resize_Sprite();
            UpdateSSneeded = true;
        }
        int cxClient = LOWORD(lParam);
        int cyClient = HIWORD(lParam);
        // Resize the status bar to fit the new client area width
        SendMessage(hStatus2, WM_SIZE, 0, MAKELPARAM(cxClient, cyClient));
        // Position the status bar at the bottom of the window
        RECT rcStatusBar;
        GetWindowRect(hStatus2, &rcStatusBar);
        int statusBarHeight = rcStatusBar.bottom - rcStatusBar.top;
        MoveWindow(hStatus2, 0, cyClient - statusBarHeight, cxClient, statusBarHeight, TRUE);
        break;
    } // no "break" here as the code in WM_MOVE is common
    case WM_CLOSE:
    {
        if (MessageBoxA(hWin, "Confirm you want to exit?", "Confirm", MB_YESNO) == IDYES)
        {
            SaveWindowPosition();
            DestroyWindow(hWin);
        }
        break;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        fDone = true;
        break;
    }
    default:
        return DefWindowProc(hWin, message, wParam, lParam);
    }
    return 0;
}

void UpdateColorSetNames(HWND hwDlg)
{
    for (UINT ti = 0; ti < 8; ti++)
    {
        if (MycRP.activeColSet[MycRP.preColSet + ti])
        {
            SetWindowTextA(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + ti), &MycRP.nameColSet[(MycRP.preColSet + ti) * 64]);
            EnableWindow(GetDlgItem(hwDlg, ti + IDC_NOMCOLSET1), TRUE);
        }
        else
        {
            char Num[8];
            sprintf_s(Num, 8, "%02i", MycRP.preColSet + ti);
            SetWindowTextA(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + ti), Num);
            EnableWindow(GetDlgItem(hwDlg, ti + IDC_NOMCOLSET1), FALSE);
        }
    }
}

void UpdateColorRotDur(HWND hwDlg)
{
    char tbuf[8];
    for (UINT ti = 0; ti < 8; ti++)
    {
        if (MycRom.ColorRotations[acFrame * 3 * MAX_COLOR_ROTATION + (ti + preColRot) * 3] != 255)
        {
            sprintf_s(tbuf, 8, "%i", MycRom.ColorRotations[acFrame * 3 * MAX_COLOR_ROTATION + (ti + preColRot) * 3 + 2]);
            SetWindowTextA(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + ti), tbuf);
            EnableWindow(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + ti), TRUE);
        }
        else
        {
            SetWindowTextA(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + ti), "");
            EnableWindow(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + ti), FALSE);
        }
    }
}

void activateColSet(int noSet)
{
    MycRP.activeColSet[noSet + MycRP.preColSet] = true;
    EnableWindow(GetDlgItem(hColSet, noSet + IDC_NOMCOLSET1), TRUE);
}

void activateColRot(int noSet)
{
    MycRom.ColorRotations[acFrame * 3 * MAX_COLOR_ROTATION + noSet * 3 + preColRot] = Ini_Gradient_Color;
    MycRom.ColorRotations[acFrame * 3 * MAX_COLOR_ROTATION + noSet * 3 + preColRot + 1] = Fin_Gradient_Color;
    MycRom.ColorRotations[acFrame * 3 * MAX_COLOR_ROTATION + noSet * 3 + preColRot + 2] = 25;
    EnableWindow(GetDlgItem(hColSet, noSet + IDC_NOMCOLSET1), TRUE);
}

void SaveColSetNames(HWND hwDlg)
{
    for (UINT ti = 0; ti < 8; ti++)
    {
        if (MycRP.activeColSet[ti + MycRP.preColSet])
        {
            GetWindowTextA(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + ti), &MycRP.nameColSet[(ti + MycRP.preColSet) * 64], 64);
        }
    }
}

LRESULT CALLBACK ColSet_Proc(HWND hwDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_MOUSEWHEEL:
        {
            if ((short)(HIWORD(wParam)) > 0) SendMessage(hwDlg, WM_VSCROLL, SB_LINEUP, (LPARAM)GetDlgItem(hwDlg, IDC_SBV));
            else SendMessage(hwDlg, WM_VSCROLL, SB_LINEDOWN, (LPARAM)GetDlgItem(hwDlg, IDC_SBV));
            return TRUE;
        }
        case WM_VSCROLL:
        {
            int acpos = GetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL);
            if (ColSetMode==0) SaveColSetNames(hwDlg);
            switch (LOWORD(wParam))
            {
            case SB_THUMBTRACK:
            case SB_THUMBPOSITION:
                SetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL, HIWORD(wParam), TRUE);
                break;
            case SB_LINEDOWN:
                if (ColSetMode == 0)
                {
                    if (acpos < MAX_COL_SETS - 8) SetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL, acpos + 1, TRUE);
                }
                else
                {
                    if (acpos < MAX_COLOR_ROTATION - 8) SetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL, acpos + 1, TRUE);
                }
                break;
            case SB_LINEUP:
                if (acpos > 0) SetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL, acpos - 1, TRUE);
                break;
            case SB_PAGEDOWN:
                if (ColSetMode == 0)
                {
                    if (acpos < MAX_COL_SETS - 8) SetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL, min(MAX_COL_SETS - 8, acpos + 8), TRUE);
                }
                else
                {
                    if (acpos < MAX_COLOR_ROTATION - 8) SetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL, min(MAX_COLOR_ROTATION - 8, acpos + 8), TRUE);
                }
                break;
            case SB_PAGEUP:
                if (acpos > 0) SetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL, max(0, acpos - 8), TRUE);
                break;
            }
            MycRP.preColSet = GetScrollPos(GetDlgItem(hwDlg, IDC_SBV), SB_CTL);
            InvalidateRect(GetDlgItem(hwDlg, IDC_COLORS),NULL, FALSE);
            if (ColSetMode == 0) UpdateColorSetNames(hwDlg); else UpdateColorRotDur(hwDlg);
            return TRUE;
        }
        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
            if (pDIS->CtlID != IDC_COLORS) return FALSE; // we only owner draw the colors
            RECT rcColors, rcFirst, rcNext;
            GetWindowRect(GetDlgItem(hwDlg, IDC_COLORS), &rcColors);
            rcColors.right -= rcColors.left;
            rcColors.bottom -= rcColors.top;
            rcColors.left = 0;
            rcColors.top = 0;
            GetWindowRect(GetDlgItem(hwDlg, IDC_SETSET1), &rcFirst);
            GetWindowRect(GetDlgItem(hwDlg, IDC_SETSET2), &rcNext);
            int colstep = rcNext.top - rcFirst.top;
            int colsize = rcFirst.bottom - rcFirst.top-2;
            HBRUSH hbr = CreateSolidBrush(RGB(255,255,255));
            FillRect(pDIS->hDC, &rcColors, hbr);
            DeleteObject(hbr);
            for (UINT tj = 0; tj < 8; tj++)
            {
                if (ColSetMode == 0)
                {
                    if (MycRP.activeColSet[tj + MycRP.preColSet])
                    {
                        for (UINT ti = 0; ti < MycRom.noColors; ti++)
                        {
                            rcFirst.left = (ti % 4) * colsize;
                            rcFirst.right = ((ti % 4) + 1) * colsize - 1;
                            rcFirst.top = tj * colstep + (ti / 4) * (colstep - 2) / 4;
                            rcFirst.bottom = rcFirst.top + (colstep - 2) / 4;
                            hbr = CreateSolidBrush(RGB(MycRom.cPal[3 * MycRom.ncColors * acFrame + MycRP.ColSets[(tj + MycRP.preColSet) * 16 + ti] * 3], MycRom.cPal[3 * MycRom.ncColors * acFrame + MycRP.ColSets[(tj + MycRP.preColSet) * 16 + ti] * 3 + 1], MycRom.cPal[3 * MycRom.ncColors * acFrame + MycRP.ColSets[(tj + MycRP.preColSet) * 16 + ti] * 3 + 2]));
                            FillRect(pDIS->hDC, &rcFirst, hbr);
                            DeleteObject(hbr);
                        }
                    }
                }
                else
                {
                    UINT8 precol = MycRom.ColorRotations[acFrame * 3 * MAX_COLOR_ROTATION + 3 * (tj + preColRot)];
                    if (precol != 255)
                    {
                        UINT8 ncol = MycRom.ColorRotations[acFrame * 3 * MAX_COLOR_ROTATION + 3 * (tj + preColRot) + 1];
                        for (UINT ti = 0; ti < ncol; ti++)
                        {
                            rcFirst.left = ti * rcColors.right / ncol;
                            rcFirst.right = (ti + 1) * rcColors.right / ncol;
                            rcFirst.top = tj * colstep;
                            rcFirst.bottom = (tj + 1) * colstep - 20;
                            hbr = CreateSolidBrush(RGB(MycRom.cPal[3 * MycRom.ncColors * acFrame + (precol + ti) * 3], MycRom.cPal[3 * MycRom.ncColors * acFrame + (precol + ti) * 3 + 1], MycRom.cPal[3 * MycRom.ncColors * acFrame + (precol + ti) * 3 + 2]));
                            FillRect(pDIS->hDC, &rcFirst, hbr);
                            DeleteObject(hbr);
                        }
                    }
                }
            }
            return TRUE;
        }
        case WM_LBUTTONDOWN:
        {
            POINT mp;
            GetCursorPos(&mp);
            RECT rcColors;
            GetWindowRect(GetDlgItem(hwDlg, IDC_COLORS), &rcColors);
            if ((mp.x < rcColors.left) || (mp.x >= rcColors.right) || (mp.y < rcColors.top) || (mp.y >= rcColors.bottom)) return FALSE; // we are not on the colors zone, so we don't manage the code
            RECT rcFirst, rcNext;
            GetWindowRect(GetDlgItem(hwDlg, IDC_SETSET1), &rcFirst);
            GetWindowRect(GetDlgItem(hwDlg, IDC_SETSET2), &rcNext);
            int colstep = rcNext.top - rcFirst.top;
            int colsize = rcFirst.bottom - rcFirst.top - 2;
            int py = mp.y - rcColors.top;
            if (py % colstep < colsize)
            {
                if (ColSetMode == 0)
                {
                    int scolset = py / colstep + MycRP.preColSet;
                    if (MycRP.activeColSet[scolset])
                    {
                        SaveAction(true, SA_EDITCOLOR);
                        for (UINT ti = 0; ti < MycRom.noColors; ti++) acEditColors[ti] = MycRP.ColSets[scolset * 16 + ti];
                        for (UINT ti = IDC_DYNACOL1; ti <= IDC_DYNACOL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
                        for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
                    }
                }
                else
                {
                    int scolset = py / colstep + preColRot;
                    //if (MycRom.ColorRotations[3 * MAX_COLOR_ROTATION * scolset] != 255)
                    {
                        //SaveAction(true, SA_COLROT);
                        acColRot = scolset;
                        InvalidateRect(GetDlgItem(hwDlg, IDC_COLORS), NULL, TRUE);
                        Ini_Gradient_Color = Fin_Gradient_Color = 255;
                        Choose_Color_Palette3();
                    }
                }
            }
            return TRUE;
        }
        case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT me{};
            me.cbSize = sizeof(TRACKMOUSEEVENT);
            me.dwFlags = TME_HOVER | TME_LEAVE;
            me.hwndTrack = hwDlg;
            me.dwHoverTime = HOVER_DEFAULT;
            TrackMouseEvent(&me);
            return TRUE;
        }
        case WM_MOUSELEAVE:
        {
            POINT mp;
            RECT rcColSet;
            GetWindowRect(hwDlg, &rcColSet);
            GetCursorPos(&mp);
            if ((mp.x > rcColSet.left) && (mp.x < rcColSet.right) && (mp.y > rcColSet.top) && (mp.y < rcColSet.bottom)) break; // we are on a child control of the dialog, but the cursor didn't leave the dialog
            SaveColSetNames(hwDlg);
            DestroyWindow(hwDlg);
            hColSet = NULL;
            return TRUE;
        }
        case WM_INITDIALOG:
        {
            if (ColSetMode == 0) SaveAction(true, SA_COLSETS);
            HWND hSB = GetDlgItem(hwDlg, IDC_SBV);
            SCROLLINFO si = { 0 };
            si.cbSize = sizeof(SCROLLINFO);
            si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
            si.nMin = 0;
            if (ColSetMode == 0) si.nMax = MAX_COL_SETS - 1; else si.nMax = MAX_COLOR_ROTATION - 1;
            si.nPage = 8;
            if (ColSetMode == 0) si.nPos = MycRP.preColSet; else si.nPos = preColRot;
            SetScrollInfo(hSB, SB_CTL, &si, TRUE);
            for (UINT ti = IDC_SETSET1; ti <= IDC_SETSET8; ti++)
            {
                if (ColSetMode == 0) SetDlgItemTextA(hwDlg, ti, "Set");
                else SetDlgItemTextA(hwDlg, ti, "Del");
            }
            if (ColSetMode == 0) UpdateColorSetNames(hwDlg); else UpdateColorRotDur(hwDlg);
            return TRUE;
        }
        case WM_COMMAND:
        {
            if ((LOWORD(wParam) >= IDC_SETSET1) && (LOWORD(wParam) <= IDC_SETSET8) && (HIWORD(wParam) == BN_CLICKED))
            {
                if (ColSetMode == 0)
                {
                    SaveAction(true, SA_COLSETS);
                    activateColSet(LOWORD(wParam) - IDC_SETSET1);
                    for (UINT ti = 0; ti < MycRom.noColors; ti++) MycRP.ColSets[(MycRP.preColSet + LOWORD(wParam) - IDC_SETSET1) * 16 + ti] = acEditColors[ti];
                    InvalidateRect(GetDlgItem(hwDlg, IDC_COLORS), NULL, TRUE);
                }
                else
                {
                    // delete rotation
                    SaveAction(true, SA_COLROT);
                    int noset = LOWORD(wParam) - IDC_SETSET1;
                    for (UINT ti = 0; ti < nSelFrames; ti++)
                    {
                        MycRom.ColorRotations[SelFrames[ti] * 3 * MAX_COLOR_ROTATION + 3 * noset] = 255;
                        MyColRot.firstcol[noset] = 255;
                        EnableWindow(GetDlgItem(hwDlg, IDC_NOMCOLSET1 + noset), FALSE);
                        SetDlgItemTextA(hwDlg, IDC_NOMCOLSET1 + noset, "");
                        InvalidateRect(GetDlgItem(hwDlg, IDC_COLORS), NULL, TRUE);
                    }
                }
                return TRUE;
            }
            else if ((LOWORD(wParam) >= IDC_NOMCOLSET1) && (LOWORD(wParam) <= IDC_NOMCOLSET8) && (HIWORD(wParam) == EN_KILLFOCUS))
            {
                SaveAction(true, SA_COLROT);
                char tbuf[256];
                GetDlgItemTextA(hwDlg, LOWORD(wParam), tbuf, 256);
                int noset = LOWORD(wParam) - IDC_NOMCOLSET1;
                int val = atoi(tbuf);
                if (val == 0) val = 5;
                if (val > 255) val = 255; // maximum 2.5s
                for (UINT ti = 0; ti < nSelFrames; ti++)
                {
                    MycRom.ColorRotations[SelFrames[ti] * 3 * MAX_COLOR_ROTATION + 3 * noset + 2] = (UINT8)val;
                }
                MyColRot.timespan[noset] = 10 * MycRom.ColorRotations[acFrame * 3 * MAX_COLOR_ROTATION + noset * 3 + 2];
            }
        }
    }

    return FALSE;
}

void ScreenToClient(HWND hwnd, RECT *prc)
{
    POINT pt;
    pt.x = prc->left;
    pt.y = prc->top;
    ScreenToClient(hwnd, &pt);
    prc->left = pt.x;
    prc->top = pt.y;
    pt.x = prc->right;
    pt.y = prc->bottom;
    ScreenToClient(hwnd, &pt);
    prc->right = pt.x;
    prc->bottom = pt.y;
}

char askname_res[SIZE_MASK_NAME];

INT_PTR AskName_Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            SetDlgItemTextA(hDlg, IDC_NAME, askname_res);
            //SetFocus(GetDlgItem(hDlg, IDC_NAME));
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch (wParam)
            {
                case IDOK:
                {
                    GetDlgItemTextA(hDlg, IDC_NAME, askname_res, SIZE_MASK_NAME - 1);
                }
                case IDCANCEL:
                {
                    EndDialog(hDlg, TRUE);
                    return wParam;
                }
            }
        }
    }
    return FALSE;
}

LRESULT CALLBACK CBList_Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
    switch (message)
    {
        case WM_RBUTTONDOWN:
        {
            if (MycRom.name[0] == 0) return TRUE;
            POINT tp;
            GetCursorPos(&tp);
            ScreenToClient(hDlg, &tp);
            int noItem=(int)SendMessage(hDlg, LB_ITEMFROMPOINT, 0, MAKELPARAM(tp.x, tp.y));
            if (noItem == 0) return TRUE;
            strcpy_s(askname_res, 64, &MycRP.Mask_Names[SIZE_MASK_NAME * (noItem - 1)]);
            if (DialogBox(hInst, MAKEINTRESOURCE(IDD_ASKNAME), hDlg, (DLGPROC)AskName_Proc) == IDOK)
            {
                strcpy_s(&MycRP.Mask_Names[SIZE_MASK_NAME * (noItem - 1)], SIZE_MASK_NAME-1, askname_res);
                UpdateMaskList();
            }
            return TRUE;
        }
    }
    return DefSubclassProc(hDlg,message,wParam,lParam);
}

HBRUSH CreateGradientBrush(UINT8 icol, UINT8 fcol, HDC hDC,LPRECT prect)
{
    HBRUSH Brush = NULL;
    HDC hdcmem = CreateCompatibleDC(hDC);
    HBITMAP hbitmap = CreateCompatibleBitmap(hDC, prect->right - prect->left, prect->bottom - prect->top);
    if (!SelectObject(hdcmem, hbitmap))
    {
        int tmp = 0;
    }

    const UINT8 fc = max(icol, fcol), ic = min(icol, fcol);
    double colwid = (double)(fc - ic + 1) / (double)(prect->right - prect->left);
    int cstart = 0;
    for (int ti = 0; ti < prect->right - prect->left; ti++)
    {
        int nsamecol = 0;
        int quelcol = min(icol, fcol) + (int)((double)ti * colwid);
        while ((ti < prect->right - prect->left) && ((int)((double)ti * colwid) == quelcol))
        {
            nsamecol++;
            ti++;
        }
        Brush = CreateSolidBrush(RGB(MycRom.cPal[acFrame * 3 * MycRom.ncColors + quelcol * 3], MycRom.cPal[acFrame * 3 * MycRom.ncColors + quelcol * 3 + 1], MycRom.cPal[acFrame * 3 * MycRom.ncColors + quelcol * 3 + 2]));
        RECT temp;
        temp.left = cstart;
        temp.top = 0;
        temp.right = cstart + nsamecol+1;
        temp.bottom = prect->bottom - prect->top;
        FillRect(hdcmem, &temp, Brush);
        DeleteObject(Brush);
        cstart += nsamecol + 1;
    }
    HBRUSH pattern = CreatePatternBrush(hbitmap);

    DeleteDC(hdcmem);
    DeleteObject(hbitmap);

    return pattern;
}

void EnableDrawButtons(void)
{
    EnableWindow(GetDlgItem(hwTB, IDC_DRAWPOINT), TRUE);
    EnableWindow(GetDlgItem(hwTB, IDC_DRAWLINE), TRUE);
    EnableWindow(GetDlgItem(hwTB, IDC_DRAWRECT), TRUE);
    EnableWindow(GetDlgItem(hwTB, IDC_DRAWCIRC), TRUE);
    EnableWindow(GetDlgItem(hwTB, IDC_FILL), TRUE);
}

void DisableDrawButtons(void)
{
    EnableWindow(GetDlgItem(hwTB, IDC_DRAWPOINT), FALSE);
    //EnableWindow(GetDlgItem(hwTB, IDC_DRAWLINE), FALSE);
    EnableWindow(GetDlgItem(hwTB, IDC_DRAWRECT), FALSE);
    //EnableWindow(GetDlgItem(hwTB, IDC_DRAWCIRC), FALSE);
    EnableWindow(GetDlgItem(hwTB, IDC_FILL), FALSE);
}

void UpdateFrameSpriteList(void)
{
    SendMessage(GetDlgItem(hwTB, IDC_SPRITELIST2), CB_RESETCONTENT, 0, 0);
    int ti = 0;
    char tbuf[256];
    while (ti < MAX_SPRITES_PER_FRAME)
    {
        if (MycRom.FrameSprites[acFrame * MAX_SPRITES_PER_FRAME + ti] == 255)
        {
            ti++;
            continue;
        }
        UINT nospr = MycRom.FrameSprites[acFrame * MAX_SPRITES_PER_FRAME + ti];
        sprintf_s(tbuf, 256, "%i - %s", nospr, &MycRP.Sprite_Names[SIZE_SECTION_NAMES * nospr]);
        SendMessageA(GetDlgItem(hwTB, IDC_SPRITELIST2), CB_ADDSTRING, 0, (LPARAM)tbuf);
        ti++;
    }
    SendMessage(GetDlgItem(hwTB, IDC_SPRITELIST2), CB_SETCURSEL, 0, 0);
}

const char* ButtonDescription(HWND hOver)
{
    if (hOver == GetDlgItem(hwTB, IDC_NEW)) return (const char*)"Create a new Serum project from a TXT dump file";
    if (hOver == GetDlgItem(hwTB, IDC_ADDTXT)) return (const char*)"Add a TXT dump file to the Serum project";
    if (hOver == GetDlgItem(hwTB, IDC_OPEN)) return (const char*)"Open a Serum project";
    if (hOver == GetDlgItem(hwTB, IDC_SAVE)) return (const char*)"Save the Serum project";
    if (hOver == GetDlgItem(hwTB, IDC_UNDO)) return (const char*)"Undo last action";
    if (hOver == GetDlgItem(hwTB, IDC_REDO)) return (const char*)"Redo last action";
    if (hOver == GetDlgItem(hwTB, IDC_SECTIONNAME)) return (const char*)"Enter the name for a new section here";
    if (hOver == GetDlgItem(hwTB, IDC_ADDSECTION)) return (const char*)"Add a section named after the text in the box beside";
    if (hOver == GetDlgItem(hwTB, IDC_DELSECTION)) return (const char*)"Delete the section";
    if (hOver == GetDlgItem(hwTB, IDC_MOVESECTION)) return (const char*)"Open a dialog to reorder your sections";
    if (Edit_Mode == 0)
    {
        if (hOver == GetDlgItem(hwTB, IDC_COLMODE)) return (const char*)"Switch to Colorization mode";
        if (hOver == GetDlgItem(hwTB, IDC_MASKLIST)) return (const char*)"Choose a mask in the list";
        if (hOver == GetDlgItem(hwTB, IDC_POINTMASK)) return (const char*)"Point tool to draw masks";
        if (hOver == GetDlgItem(hwTB, IDC_RECTMASK)) return (const char*)"Filled-rectangle tool to draw masks";
        if (hOver == GetDlgItem(hwTB, IDC_ZONEMASK)) return (const char*)"Magic wand tool to draw masks";
        if (hOver == GetDlgItem(hwTB, IDC_INVERTSEL)) return (const char*)"Invert the mask selection";
        if (hOver == GetDlgItem(hwTB, IDC_SHAPEMODE)) return (const char*)"Switch to shape mode";
        if (hOver == GetDlgItem(hwTB, IDC_ZONEMASK)) return (const char*)"Magic wand tool to draw masks";
        if (hOver == GetDlgItem(hwTB, IDC_SAMEFRAME)) return (const char*)"How many same frames detected";
        if (hOver == GetDlgItem(hwTB, IDC_DELSAMEFR)) return (const char*)"Delete frames similar to the current displayed one taking into account masks and shape mode";
        if (hOver == GetDlgItem(hwTB, IDC_DELSELSAMEFR)) return (const char*)"Delete frames similar to the current selected ones taking into account masks and shape mode";
        if (hOver == GetDlgItem(hwTB, IDC_DELALLSAMEFR)) return (const char*)"Delete all the frames similar taking into account masks and shape mode";
        if (hOver == GetDlgItem(hwTB, IDC_DELFRAME)) return (const char*)"Delete selected frames";
        if (hOver == GetDlgItem(hwTB, IDC_TRIGID)) return (const char*)"[PuP packs] Trigger ID for an event";
        if (hOver == GetDlgItem(hwTB, IDC_DELTID)) return (const char*)"[PuP packs] Delete the trigger";
        if (hOver == GetDlgItem(hwTB, IDC_MASKLIST2)) return (const char*)"Choose a mask here to list the frames using it below";
        if (hOver == GetDlgItem(hwTB, IDC_MOVESECTION)) return (const char*)"List of frames using the mask above";
        if (hOver == GetDlgItem(hwTB, IDC_SECTIONLIST)) return (const char*)"Choose a section here to jump to its first frame";
    }
    else
    {
        if (hOver == GetDlgItem(hwTB, IDC_ORGMODE)) return (const char*)"Switch to Comparison mode";
        for (int ti = 0; ti < 16; ti++)
        {
            if (hOver == GetDlgItem(hwTB, IDC_COL1 + ti))
            {
                char tbuf[512];
                sprintf_s(tbuf, 512, "Select the color #%i for drawing in the 64-colour palette", ti + 1);
                return tbuf;
            }
            if (hOver == GetDlgItem(hwTB, IDC_DYNACOL1 + ti))
            {
                char tbuf[512];
                sprintf_s(tbuf, 512, "Select the color #%i for dynamic colorization in the 64-colour palette", ti + 1);
                return tbuf;
            }
        }
        if (hOver == GetDlgItem(hwTB, IDC_ORGMODE)) return (const char*)"Switch to Comparison mode";
        if (hOver == GetDlgItem(hwTB, IDC_COLSET)) return (const char*)"Show a dialog box to manage the color sets";
        if (hOver == GetDlgItem(hwTB, IDC_COLPICK)) return (const char*)"Pick a color from the displayed frame";
        if (hOver == GetDlgItem(hwTB, IDC_1COLMODE)) return (const char*)"Draw using a single color";
        if (hOver == GetDlgItem(hwTB, IDC_4COLMODE)) return (const char*)"Draw replacing the original frame with the selected set of colors";
        if (hOver == GetDlgItem(hwTB, IDC_GRADMODE)) return (const char*)"Draw using gradients";
        if (hOver == GetDlgItem(hwTB, IDC_GRADMODEB)) return (const char*)"Draw using gradients";
        if (hOver == GetDlgItem(hwTB, IDC_DRAWPOINT)) return (const char*)"Point tool to draw or select";
        if (hOver == GetDlgItem(hwTB, IDC_DRAWLINE)) return (const char*)"Line tool to draw or select";
        if (hOver == GetDlgItem(hwTB, IDC_DRAWRECT)) return (const char*)"Rectangle tool to draw or select";
        if (hOver == GetDlgItem(hwTB, IDC_DRAWCIRC)) return (const char*)"Circle tool to draw or select";
        if (hOver == GetDlgItem(hwTB, IDC_FILL)) return (const char*)"Magic wand tool to draw or select";
        if (hOver == GetDlgItem(hwTB, IDC_FILLED)) return (const char*)"Draw/select rectangles or circles are filled or not?";
        if (hOver == GetDlgItem(hwTB, IDC_COPYCOLS)) return (const char*)"Copy the color set from the displayed frame to the other selected ones";
        if (hOver == GetDlgItem(hwTB, IDC_COPYCOLS2)) return (const char*)"Copy the whole 64-colour palette from the displayed frame to the other selected ones";
        if (hOver == GetDlgItem(hwTB, IDC_CHANGECOLSET)) return (const char*)"Switch between the different dynamic color sets";
        if (hOver == GetDlgItem(hwTB, IDC_COLTODYNA)) return (const char*)"Copy the color set for drawing to the current dynamic set";
        if (hOver == GetDlgItem(hwTB, IDC_IDENT)) return (const char*)"Identify the points on the displayed frame where the active dynamic set is used";
        if (hOver == GetDlgItem(hwTB, IDC_COPY)) return (const char*)"Copy selected content";
        if (hOver == GetDlgItem(hwTB, IDC_PASTE)) return (const char*)"Paste selected content";
        if (hOver == GetDlgItem(hwTB, IDC_INVERTSEL2)) return (const char*)"Invert selection for copy";
        if (hOver == GetDlgItem(hwTB, IDC_SECTIONLIST2)) return (const char*)"Choose a section here to jump to its first frame";
        if (hOver == GetDlgItem(hwTB, IDC_ADDSPRITE2)) return (const char*)"Add the current sprite in the sprite window to the list of sprites to be detected when this frame is found";
        if (hOver == GetDlgItem(hwTB, IDC_DELSPRITE2)) return (const char*)"Delete the current sprite in the sprite window from the list of sprites to be detected when this frame is found";
        if (hOver == GetDlgItem(hwTB, IDC_SPRITELIST2)) return (const char*)"List of sprites to be detected when this frame is found";
        if (hOver == GetDlgItem(hwTB, IDC_COLROT)) return (const char*)"Display a dialog to manage the color rotations";
    }
    return "";
}

LRESULT CALLBACK ButtonSubclassProc(HWND hBut, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (message) {
        case WM_KEYDOWN:
        {
            return TRUE;
        }
        case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hBut;
            tme.dwHoverTime = HOVER_DEFAULT;
            TrackMouseEvent(&tme);
            SetWindowTextA(hStatus, ButtonDescription(hBut));
            break;
        }
        case WM_MOUSELEAVE:
        {
            // The mouse has left the button
            SetWindowTextA(hStatus, "");
            break;
        }
    }

    // Call the default button window procedure for any unhandled messages
    return DefSubclassProc(hBut, message, wParam, lParam);
}

INT_PTR CALLBACK Toolbar_Proc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    //UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        case WM_INITDIALOG:
        {
            /*HWND hwndChild = GetWindow(hDlg, GW_CHILD); // get the first child window
            while (hwndChild != NULL)
            {*/
            SetWindowSubclass(GetDlgItem(hDlg, IDC_NEW), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_ADDTXT), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_OPEN), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_SAVE), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_UNDO), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_REDO), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_SECTIONNAME), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_ADDSECTION), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_DELSECTION), ButtonSubclassProc, 0, 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDC_MOVESECTION), ButtonSubclassProc, 0, 0);
            if (Edit_Mode == 0)
            {
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COLMODE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_MASKLIST), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_POINTMASK), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_RECTMASK), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_ZONEMASK), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_INVERTSEL), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_SHAPEMODE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_SAMEFRAME), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DELFRAME), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DELSAMEFR), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DELSELSAMEFR), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DELALLSAMEFR), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_TRIGID), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DELTID), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_MASKLIST2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_MOVESECTION), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_SECTIONLIST), ButtonSubclassProc, 0, 0);
            }
            else
            {
                SetWindowSubclass(GetDlgItem(hDlg, IDC_ORGMODE), ButtonSubclassProc, 0, 0);
                for (int ti = 0; ti < 16; ti++)
                {
                    SetWindowSubclass(GetDlgItem(hDlg, IDC_COL1 + ti), ButtonSubclassProc, 0, 0);
                    SetWindowSubclass(GetDlgItem(hDlg, IDC_DYNACOL1 + ti), ButtonSubclassProc, 0, 0);
                    //char tbuf[512];
                    //sprintf_s(tbuf, 512, "(Left click)Select/(Right click)Modify the color #%i of the palette for drawing", ti+1);
                }
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COLSET), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COLPICK), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_1COLMODE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_4COLMODE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_GRADMODE), ButtonSubclassProc, 0, 0);
                //SetWindowSubclass(GetDlgItem(hDlg, IDC_GRADMODEB), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWPOINT), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWLINE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWRECT), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWCIRC), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_FILL), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_FILLED), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COPYCOLS), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COPYCOLS2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_CHANGECOLSET), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COLTODYNA), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_IDENT), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COPY), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_PASTE), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_INVERTSEL2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_SECTIONLIST2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_ADDSPRITE2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_DELSPRITE2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_SPRITELIST2), ButtonSubclassProc, 0, 0);
                SetWindowSubclass(GetDlgItem(hDlg, IDC_COLROT), ButtonSubclassProc, 0, 0);

                if (MycRP.DrawColMode == 1)
                {
                    SendMessage(GetDlgItem(hDlg, IDC_1COLMODE), BM_SETCHECK, FALSE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_4COLMODE), BM_SETCHECK, TRUE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_GRADMODE), BM_SETCHECK, FALSE, 0);
                }
                else if (MycRP.DrawColMode == 0)
                {
                    SendMessage(GetDlgItem(hDlg, IDC_4COLMODE), BM_SETCHECK, FALSE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_1COLMODE), BM_SETCHECK, TRUE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_GRADMODE), BM_SETCHECK, FALSE, 0);
                }
                else
                {
                    SendMessage(GetDlgItem(hDlg, IDC_4COLMODE), BM_SETCHECK, FALSE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_1COLMODE), BM_SETCHECK, FALSE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_GRADMODE), BM_SETCHECK, TRUE, 0);
                    if (MycRP.Fill_Mode == TRUE) Button_SetCheck(GetDlgItem(hwTB, IDC_FILLED), BST_CHECKED); else Button_SetCheck(GetDlgItem(hwTB, IDC_FILLED), BST_UNCHECKED);
                }
            }
            return TRUE;
        }
        case WM_CTLCOLORDLG:
        {
            if (Night_Mode) return (INT_PTR)GetStockObject(DKGRAY_BRUSH);
            return (INT_PTR)GetStockObject(GRAY_BRUSH);
        }
        case WM_PAINT:
        {
            if (MycRom.name[0] == 0) return FALSE;
            if (Edit_Mode == 1)
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hDlg, &ps);
                RECT rc;
                GetWindowRect(GetDlgItem(hDlg, IDC_DRAWPOINT + MycRP.Draw_Mode), &rc);
                rc.bottom += 5;
                HBRUSH br = CreateSolidBrush(RGB(255, 0, 0));
                ScreenToClient(hDlg, &rc);
                FillRect(hdc, &rc, br);
/*                GetWindowRect(GetDlgItem(hDlg, IDC_COL1 + noColSel), &rc);
                rc.top -= 2;
                rc.left -= 2;
                rc.bottom += 2;
                rc.right += 2;
                ScreenToClient(hDlg, &rc);
                FillRect(hdc, &rc, br);*/
                DeleteObject(br);
                EndPaint(hDlg, &ps);
                return TRUE;
            }
            else
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hDlg, &ps);
                RECT rc;
                GetWindowRect(GetDlgItem(hDlg, IDC_POINTMASK + MycRP.Mask_Sel_Mode), &rc);
                rc.bottom += 5;
                HBRUSH br = CreateSolidBrush(RGB(255, 0, 0));
                ScreenToClient(hDlg, &rc);
                FillRect(hdc, &rc, br);
                DeleteObject(br);
                EndPaint(hDlg, &ps);
                return TRUE;
            }
            return FALSE;
        }
        case WM_DRAWITEM:
        {
            if ((MycRom.cPal) && (Edit_Mode == 1))
            {
                LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
                UINT tpm;
                if ((lpdis->CtlID >= IDC_COL1) && (lpdis->CtlID < IDC_COL1 + MycRom.noColors))
                {
                    if (lpdis->CtlID)
                        tpm = lpdis->CtlID - IDC_COL1;
                    HBRUSH bg = CreateSolidBrush(RGB(MycRom.cPal[acFrame * 3 * 64 + acEditColors[tpm] * 3], MycRom.cPal[acFrame * 3 * 64 + acEditColors[tpm] * 3 + 1], MycRom.cPal[acFrame * 3 * 64 + acEditColors[tpm] * 3 + 2]));
                    RECT rc = lpdis->rcItem;
                    if (tpm == noColSel)
                    {
                        HBRUSH br = CreateSolidBrush(RGB(255, 0, 0));
                        FillRect(lpdis->hDC, &rc, br);
                        DeleteObject(br);
                    }
                    rc.left += 2;
                    rc.right -= 2;
                    rc.top += 2;
                    rc.bottom -= 2;
                    FillRect(lpdis->hDC, &rc, bg);
                    DeleteObject(bg);
                }
                else if ((lpdis->CtlID >= IDC_DYNACOL1) && (lpdis->CtlID < IDC_DYNACOL1 + MycRom.noColors))
                {
                    tpm = lpdis->CtlID - IDC_DYNACOL1;
                    HBRUSH bg = CreateSolidBrush(RGB(MycRom.cPal[acFrame * 3 * 64 + MycRom.Dyna4Cols[(acFrame * MAX_DYNA_SETS_PER_FRAME + acDynaSet) * MycRom.noColors + tpm] * 3], MycRom.cPal[acFrame * 3 * 64 + MycRom.Dyna4Cols[(acFrame * MAX_DYNA_SETS_PER_FRAME + acDynaSet) * MycRom.noColors + tpm] * 3 + 1], MycRom.cPal[acFrame * 3 * 64 + MycRom.Dyna4Cols[(acFrame * MAX_DYNA_SETS_PER_FRAME + acDynaSet) * MycRom.noColors + tpm] * 3 + 2]));
                    RECT rc = lpdis->rcItem;
                    rc.left += 2;
                    rc.right -= 2;
                    rc.top += 2;
                    rc.bottom -= 2;
                    FillRect(lpdis->hDC, &rc, bg);
                    DeleteObject(bg);
                }
                else if (lpdis->CtlID == IDC_GRADMODEB)
                {
                    RECT tr;
                    tr.left = lpdis->rcItem.left;
                    tr.right = lpdis->rcItem.right;
                    tr.top = lpdis->rcItem.top;
                    tr.bottom = lpdis->rcItem.bottom;
                    HBRUSH bg = CreateGradientBrush(Draw_Grad_Ini, Draw_Grad_Fin, lpdis->hDC, & tr);
                    FillRect(lpdis->hDC, &tr, bg);
                    DeleteObject(bg);
                    SetTextColor(lpdis->hDC, RGB(SelFrameColor, SelFrameColor, SelFrameColor));
                    SetBkMode(lpdis->hDC, TRANSPARENT);
                    TextOutA(lpdis->hDC, 24, 0, "Gradient draw", (int)strlen("Gradient draw"));
                }
            }
            return TRUE;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_NEW:
                {
                    Load_TXT_File();
                    Calc_Resize_Frame();
                    UpdateFSneeded = true;
                    Update_Toolbar = true;// CreateToolbar();
                    Update_Toolbar2 = true;
                    return TRUE;
                }
                case IDC_ADDTXT:
                {
                    if (MycRom.name[0])
                    {
                        Add_TXT_File();
                        Calc_Resize_Frame();
                        UpdateFSneeded = true;
                        Update_Toolbar = true;// CreateToolbar();
                        Update_Toolbar2 = true;
                    }
                    return TRUE;
                }
                case IDC_ORGMODE:
                {
                    if (hPal)
                    {
                        DestroyWindow(hPal);
                        hPal = NULL;
                    }
                    if (hColSet)
                    {
                        DestroyWindow(hColSet);
                        hColSet = NULL;
                    }
                    Edit_Mode = 0;
                    UpdateFSneeded = true;
                    Update_Toolbar = true;// CreateToolbar();
                    glfwSetCursor(glfwframe, glfwarrowcur);
                    SetCursor(hcArrow);
                    Color_Pipette = false;
                    Paste_Mode = false;
                    return TRUE;
                }
                case IDC_COLMODE:
                {
                    Edit_Mode = 1;
                    UpdateFSneeded = true;
                    Update_Toolbar = true;// CreateToolbar();
                    return TRUE;
                }
                case IDC_SAVE:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (Save_cRom(false))
                    {
                        cprintf("%s.cROM saved in %s", MycRom.name, MycRP.SaveDir);
                        if (Save_cRP(false)) cprintf("%s.cRP saved in %s", MycRom.name, MycRP.SaveDir);
                    }
                    return TRUE;
                }
                case IDC_OPEN:
                {
                    char acDir[260],acFile[260];
                    GetCurrentDirectoryA(260, acDir);
                    OPENFILENAMEA ofn;
                    char szFile[260] = { 0 };

                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.lpstrTitle = "Choose the project file";
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFile = szFile;
                    ofn.lpstrFile[0] = '\0';
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = "Colorized ROM\0*.cROM\0";
                    ofn.nFilterIndex = 1;
                    ofn.lpstrInitialDir = MycRP.SaveDir;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                    if (GetOpenFileNameA(&ofn) == TRUE)
                    {
                        if (isLoadedProject)
                        {
                            if (MessageBox(hWnd, L"Confirm you want to close the current project and load a new one", L"Caution", MB_YESNO) == IDYES)
                            {
                                Free_Project();
                            }
                            else
                            {
                                SetCurrentDirectoryA(acDir);
                                return TRUE;
                            }
                        }
                        strcpy_s(acFile, 260, ofn.lpstrFile);
                        if (strstr(acFile, "(auto)") != NULL)
                        {
                            if (MessageBoxA(hWnd, "This cRom was automatically saved, if you manually save it, it will overwrite any previously manually saved file on the same rom, even if it was more recent. Do you still want to open it?", "Caution", MB_YESNO) == IDNO) return TRUE;
                        }
                        if (!Load_cRom(acFile)) // crom must be loaded before crp
                        {
                            Free_Project();
                            return FALSE;
                        }
                        int endst = (int)strlen(acFile);
                        while ((acFile[endst] != '.') && (endst > 0)) endst--;
                        acFile[endst] = 0;
                        strcat_s(acFile,260, ".cRP");
                        if (!Load_cRP(acFile)) Free_Project();
                        else cprintf("The cRom has been loaded.");
                        SetCurrentDirectoryA(acDir);
                        Init_cFrame_Palette2();
                        Calc_Resize_Frame();
                        InitColorRotation();
                        UpdateFSneeded = true;
                        UpdateSSneeded = true;
                        Update_Toolbar = true;// CreateToolbar();
                        Update_Toolbar2 = true;
                        UpdateSpriteList3();
                    }
                    return TRUE;
                }
                case IDC_COL1:
                case IDC_COL2:
                case IDC_COL3:
                case IDC_COL4:
                case IDC_COL5:
                case IDC_COL6:
                case IDC_COL7:
                case IDC_COL8:
                case IDC_COL9:
                case IDC_COL10:
                case IDC_COL11:
                case IDC_COL12:
                case IDC_COL13:
                case IDC_COL14:
                case IDC_COL15:
                case IDC_COL16:
                {
                    if (LOWORD(wParam) >= IDC_COL1 + MycRom.noColors) return TRUE;
                    noColMod = noColSel = LOWORD(wParam) - IDC_COL1;
                    InvalidateRect(hwTB, NULL, TRUE);
                    if (MycRom.name[0] == 0) return TRUE;
                    if (hColSet)
                    {
                        DestroyWindow(hColSet);
                        hColSet = NULL;
                    }
                    Choose_Color_Palette(LOWORD(wParam));
                    return TRUE;
                }
                case IDC_COLSET:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (hPal)
                    {
                        DestroyWindow(hPal);
                        hPal = NULL;
                    }
                    ColSetMode = 0;
                    if (hColSet) DestroyWindow(hColSet);
                    hColSet = CreateDialog(hInst, MAKEINTRESOURCE(IDD_COLSET), hDlg, (DLGPROC)ColSet_Proc);
                    if (!hColSet)
                    {
                        AffLastError((char*)"ColSet dialog display");
                        break;
                    }
                    RECT rc;
                    GetWindowRect(GetDlgItem(hwTB, IDC_COLSET), &rc);
                    SetWindowPos(hColSet, 0, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    ShowWindow(hColSet, TRUE);
                    return TRUE;
                }
                case IDC_1COLMODE:
                {
                    MycRP.DrawColMode = 0;
                    EnableDrawButtons();
                    return TRUE;
                }
                case IDC_4COLMODE:
                {
                    MycRP.DrawColMode = 1;
                    EnableDrawButtons();
                    return TRUE;
                }
                case IDC_GRADMODE:
                {
                    MycRP.DrawColMode = 2;
                    if ((MycRP.Draw_Mode != 1) && (MycRP.Draw_Mode != 3)) MycRP.Draw_Mode = 1;
                    InvalidateRect(hwTB, NULL, TRUE);
                    DisableDrawButtons();
                    return TRUE;
                }
                case IDC_GRADMODEB:
                {
                    MycRP.DrawColMode = 2;
                    SendMessage(GetDlgItem(hDlg, IDC_4COLMODE), BM_SETCHECK, FALSE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_1COLMODE), BM_SETCHECK, FALSE, 0);
                    SendMessage(GetDlgItem(hDlg, IDC_GRADMODE), BM_SETCHECK, TRUE, 0);
                    if ((MycRP.Draw_Mode!=1)&&(MycRP.Draw_Mode != 3)) MycRP.Draw_Mode = 1;
                    InvalidateRect(hwTB, NULL, TRUE);
                    DisableDrawButtons();
                    return TRUE;
                }
                case IDC_UNDO:
                {
                    RecoverAction(true);
                    return TRUE;
                }
                case IDC_REDO:
                {
                    RecoverAction(false); 
                    return TRUE;
                }
                case IDC_DRAWPOINT:
                case IDC_DRAWLINE:
                case IDC_DRAWRECT:
                case IDC_DRAWCIRC:
                case IDC_FILL:
                {
                    MycRP.Draw_Mode = (UINT8)(LOWORD(wParam) - IDC_DRAWPOINT);
                    InvalidateRect(hDlg, NULL, TRUE);
                    return TRUE;
                }
                case IDC_FILLED:
                {
                    if (Button_GetCheck(GetDlgItem(hwTB, IDC_FILLED)) == BST_CHECKED) MycRP.Fill_Mode = TRUE; else MycRP.Fill_Mode = FALSE;
                    return TRUE;
                }
                case IDC_COPYCOLS:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SaveAction(true, SA_PALETTE);
                    for (UINT ti = 0; ti < nSelFrames; ti++)
                    {
                        if (SelFrames[ti] == acFrame) continue;
                        for (UINT tj = 0; tj < 4; tj++)
                        {
                            MycRom.cPal[SelFrames[ti] * 3 * MycRom.ncColors + 3 * acEditColors[tj]] = MycRom.cPal[acFrame * 3 * MycRom.ncColors + 3 * acEditColors[tj]];
                            MycRom.cPal[SelFrames[ti] * 3 * MycRom.ncColors + 3 * acEditColors[tj] + 1] = MycRom.cPal[acFrame * 3 * MycRom.ncColors + 3 * acEditColors[tj] + 1];
                            MycRom.cPal[SelFrames[ti] * 3 * MycRom.ncColors + 3 * acEditColors[tj] + 2] = MycRom.cPal[acFrame * 3 * MycRom.ncColors + 3 * acEditColors[tj] + 2];
                        }
                    }
                    return TRUE;
                }
                case IDC_COPYCOLS2:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SaveAction(true, SA_PALETTE);
                    SaveAction(true, SA_DYNACOLOR);
                    for (UINT ti = 0; ti < nSelFrames; ti++)
                    {
                        if (SelFrames[ti] == acFrame) continue;
                        memcpy(&MycRom.cPal[SelFrames[ti] * 3 * MycRom.ncColors], &MycRom.cPal[acFrame * 3 * MycRom.ncColors], 3 * MycRom.ncColors);
                        memcpy(&MycRom.Dyna4Cols[SelFrames[ti] * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors], &MycRom.Dyna4Cols[acFrame * MAX_DYNA_SETS_PER_FRAME * MycRom.noColors], MAX_DYNA_SETS_PER_FRAME* MycRom.noColors);
                    }
                    return TRUE;
                }
                case IDC_COLPICK:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (Color_Pipette) Color_Pipette = false; else Color_Pipette = true;
                    Mouse_Mode = 0;
                    if (Color_Pipette)
                    {
                        glfwSetCursor(glfwframe, glfwdropcur);
                        SetCursor(hcColPick);
                    }
                    else
                    {
                        glfwSetCursor(glfwframe, glfwarrowcur);
                        SetCursor(hcArrow);
                    }
                    return TRUE;
                }
                case IDC_POINTMASK:
                {
                    MycRP.Mask_Sel_Mode = 0;
                    InvalidateRect(hDlg, NULL, TRUE);
                    return TRUE;
                }
                case IDC_RECTMASK:
                {
                    MycRP.Mask_Sel_Mode = 1;
                    MouseIniPosx = -1;
                    InvalidateRect(hDlg, NULL, TRUE);
                    return TRUE;
                }
                case IDC_ZONEMASK:
                {
                    MycRP.Mask_Sel_Mode = 2;
                    InvalidateRect(hDlg, NULL, TRUE);
                    return TRUE;
                }
                case IDC_MASKLIST:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    unsigned char acpos = (unsigned char)SendMessage(GetDlgItem(hDlg, IDC_MASKLIST), CB_GETCURSEL, 0, 0) - 1;
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        SaveAction(true, SA_MASKID);
                        if ((acpos != 255) && (acpos + 1 > (UINT8)MycRom.nCompMasks)) MycRom.nCompMasks = acpos + 1;
                        if (nSelFrames > 1)
                        {
                            if (MessageBox(hWnd, L"Change the mask ID for all the selected frames (\"Yes\") or just the current frame (\"No\")", L"Confirmation", MB_YESNO) == IDYES)
                            {
                                for (UINT32 ti = 0; ti < nSelFrames; ti++)
                                    MycRom.CompMaskID[SelFrames[ti]] = acpos;
                                UpdateMaskList();
                                CheckSameFrames();
                                UpdateFSneeded = true;
                                return TRUE;
                            }
                        }
                        MycRom.CompMaskID[acFrame] = acpos;
                        SetDlgItemTextA(hDlg, IDC_MASKLIST, &MycRP.Mask_Names[acpos * SIZE_MASK_NAME]);
                        CheckSameFrames();
                        UpdateFSneeded = true;
                        UpdateMaskList();
                        return TRUE;
                    }
                    return TRUE;
                }
                case IDC_MASKLIST2:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    unsigned char acpos = (unsigned char)SendMessage(GetDlgItem(hDlg, IDC_MASKLIST2), CB_GETCURSEL, 0, 0) - 1;
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        UpdateMaskList2();
                    }
                    return TRUE;
                }
                case IDC_DYNACOL1:
                case IDC_DYNACOL2:
                case IDC_DYNACOL3:
                case IDC_DYNACOL4:
                case IDC_DYNACOL5:
                case IDC_DYNACOL6:
                case IDC_DYNACOL7:
                case IDC_DYNACOL8:
                case IDC_DYNACOL9:
                case IDC_DYNACOL10:
                case IDC_DYNACOL11:
                case IDC_DYNACOL12:
                case IDC_DYNACOL13:
                case IDC_DYNACOL14:
                case IDC_DYNACOL15:
                case IDC_DYNACOL16:
                {
                    if (LOWORD(wParam) >= IDC_DYNACOL1 + MycRom.noColors) return TRUE;
                    if (MycRom.name[0] == 0) return TRUE;
                    if (hColSet)
                    {
                        DestroyWindow(hColSet);
                        hColSet = NULL;
                    }
                    noColMod = 16 + LOWORD(wParam) - IDC_DYNACOL1;
                    Choose_Color_Palette(LOWORD(wParam));
                    return TRUE;
                }
                case IDC_DELSAMEFR:
                {
                    if ((MycRom.name[0] == 0) || (nSameFrames == 0)) return TRUE;
                    if (MessageBox(hDlg, L"This action can't be undone, are you sure you want to delete the frames similar to the current one?\r\nBe aware that if your comparison mask is not correctly/accurately drawn, you may lost some frames that you still need.", L"Confirm", MB_YESNO) == IDYES)
                    {
                        int inisfr = nSameFrames;
                        while (nSameFrames > 0)
                        {
                            Display_Avancement((float)(inisfr - nSameFrames) / (float)nSameFrames,0,1);
                            Delete_Frame(SameFrames[0]);
                        }
                        UpdateSectionList();
                        UpdateFSneeded = true;
                        SetDlgItemText(hDlg, IDC_SAMEFRAME, L"0");
                    }
                    return TRUE;
                }
                case IDC_DELSELSAMEFR:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (MessageBox(hDlg, L"This action can't be undone, are you sure you want to delete the frames similar to the current one?\r\nBe aware that if your comparison mask is not correctly/accurately drawn, you may lost some frames that you still need.", L"Confirm", MB_YESNO) == IDYES)
                    {
                        int nDelFrames = 0;
                        for (UINT ti = 0; ti < nSelFrames; ti++)
                        {
                            acFrame = SelFrames[ti];
                            CheckSameFrames();
                            int inisfr = nSameFrames;
                            Display_Avancement((float)ti / (float)nSelFrames,0,1);
                            while (nSameFrames > 0)
                            {
                                Delete_Frame(SameFrames[0]);
                                nDelFrames++;
                            }
                            acFrame++;
                        }
                        if (!isFrameSelected2(acFrame))
                        {
                            nSelFrames = 1;
                            SelFrames[0] = acFrame;
                        }
                        if (acFrame >= MycRom.nFrames) acFrame = MycRom.nFrames - 1;
                        UpdateNewacFrame();
                        UpdateSectionList();
                        UpdateFSneeded = true;
                        acFrame = SelFrames[0];
                        InitColorRotation();
                        SetDlgItemText(hDlg, IDC_SAMEFRAME, L"0");
                        cprintf("%i frames were deleted as duplicate frames using the comparison masks", nDelFrames);
                    }
                    return TRUE;
                }
                case IDC_DELALLSAMEFR:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (MessageBox(hDlg, L"CAUTION!!!!! This action can't be undone, are you sure you want to delete all the similar frames?\r\nBe aware that if your comparison masks are not correctly/accurately drawn, you may lost some frames that you still need.", L"Confirm", MB_YESNO) == IDYES)
                    {
                        acFrame = 0;
                        int nDelFrames = 0;
                        while (acFrame < MycRom.nFrames)
                        {
                            CheckSameFrames();
                            int inisfr = nSameFrames;
                            Display_Avancement((float)acFrame / (float)MycRom.nFrames,0,1);
                            while (nSameFrames > 0)
                            {
                                Delete_Frame(SameFrames[0]);
                                nDelFrames++;
                            }
                            acFrame++;
                        }
                        if (!isFrameSelected2(acFrame))
                        {
                            nSelFrames = 1;
                            SelFrames[0] = acFrame;
                        }
                        if (acFrame >= MycRom.nFrames) acFrame = MycRom.nFrames - 1;
                        UpdateNewacFrame();
                        UpdateSectionList();
                        UpdateFSneeded = true;
                        acFrame = 0;
                        InitColorRotation();
                        SetDlgItemText(hDlg, IDC_SAMEFRAME, L"0");
                        cprintf("%i frames were deleted as duplicate frames using the comparison masks", nDelFrames);
                    }
                    return TRUE;
                }
                case IDC_DELFRAME:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (MessageBox(hDlg, L"This action can't be undone, are you sure you want to delete those frames?", L"Confirm", MB_YESNO) == IDYES)
                    {
                        int inisfr = nSelFrames;
                        while (nSelFrames > 0)
                        {
                            Display_Avancement((float)(inisfr - nSelFrames) / (float)nSelFrames,0,1);
                            Delete_Frame(SelFrames[0]);
                        }
                        UpdateSectionList();
                        UpdateFSneeded = true;
                        acFrame = PreFrameInStrip;
                        UpdateNewacFrame();
                        InitColorRotation();
                    }
                    return TRUE;
                }
                case IDC_SHAPEMODE:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SaveAction(true, SA_SHAPEMODE);
                    if (SendMessage(GetDlgItem(hwTB, IDC_SHAPEMODE), BM_GETCHECK, 0, 0) == BST_CHECKED)
                    {
                        for (unsigned int ti = 0; ti < nSelFrames; ti++) MycRom.ShapeCompMode[SelFrames[ti]] = TRUE;
                    }
                    else
                    {
                        for (unsigned int ti = 0; ti < nSelFrames; ti++) MycRom.ShapeCompMode[SelFrames[ti]] = FALSE;
                    }
                    CheckSameFrames();
                    return TRUE;
                }
                case IDC_COPY:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    bool datafound = false;
                    int cminx = MycRom.fWidth, cmaxx = -1;
                    int cminy = MycRom.fHeight, cmaxy = -1;
                    for (int tj = 0; tj < (int)MycRom.fHeight; tj++)
                    {
                        for (int ti = 0; ti < (int)MycRom.fWidth; ti++)
                        {
                            if (Copy_Mask[ti+tj*MycRom.fWidth] > 0)
                            {
                                Copy_Col[ti + tj * MycRom.fWidth] = MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti + tj * MycRom.fWidth];
                                Copy_Colo[ti + tj * MycRom.fWidth] = MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti + tj * MycRom.fWidth];
                                Copy_Dyna[ti + tj * MycRom.fWidth] = MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight + ti + tj * MycRom.fWidth];
                                datafound = true;
                                if (ti > cmaxx) cmaxx = ti;
                                if (ti < cminx) cminx = ti;
                                if (tj > cmaxy) cmaxy = tj;
                                if (tj < cminy) cminy = tj;
                            }
                        }
                    }
                    if (!datafound) return TRUE;
                    Paste_Width = cmaxx - cminx + 1;
                    Paste_Height = cmaxy - cminy + 1;
                    for (int tj = cminy; tj <= cmaxy; tj++)
                    {
                        for (int ti = cminx; ti <= cmaxx; ti++)
                        {
                            Paste_Mask[ti - cminx + (tj - cminy) * Paste_Width] = Copy_Mask[ti + tj * MycRom.fWidth];
                            Paste_Col[ti - cminx + (tj - cminy) * Paste_Width] = Copy_Col[ti + tj * MycRom.fWidth];
                            Paste_Dyna[ti - cminx + (tj - cminy) * Paste_Width] = Copy_Dyna[ti + tj * MycRom.fWidth];
                        }
                    }
                    Copy_Available = true;
                    Copy_From_Frame = acFrame;
                    EnableWindow(GetDlgItem(hwTB, IDC_PASTE), TRUE);
                    EnableWindow(GetDlgItem(hwTB2, IDC_PASTE), TRUE);
                    return TRUE;
                }
                case IDC_PASTE:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (Paste_Mode) Paste_Mode = false; else Paste_Mode = true;
                    Mouse_Mode = 0;
                    if (Paste_Mode)
                    {
                        glfwSetCursor(glfwframe, glfwpastecur);
                        SetCursor(hcPaste);
                    }
                    else
                    {
                        glfwSetCursor(glfwframe, glfwarrowcur);
                        SetCursor(hcArrow);
                        return TRUE;
                    }
                    Paste_Mirror = 0;
                    if (GetKeyState('K') & 0x8000) Paste_Mirror |= 1;
                    if (GetKeyState('L') & 0x8000) Paste_Mirror |= 2;
                    //                   MouseFrameLPressed = MouseFrameRPressed = MouseFrSliderLPressed = false;
                    int xmin = 257, xmax = -1, ymin = 65, ymax = -1;
                    for (int tj = 0; tj < (int)Paste_Height; tj++)
                    {
                        for (int ti = 0; ti < (int)Paste_Width; ti++)
                        {
                            if (Paste_Mask[ti + tj * Paste_Width] == 0) continue;
                            if (ti < xmin) xmin = ti;
                            if (ti > xmax) xmax = ti;
                            if (tj < ymin) ymin = tj;
                            if (tj > ymax) ymax = tj;
                        }
                    }
                    int tx, ty;
                    glfwGetWindowPos(glfwframe, &tx, &ty);
                    paste_centerx = tx + frame_zoom * (xmax + xmin) / 2;
                    paste_centery = ty + frame_zoom * (ymax + ymin) / 2;
                    return TRUE;
                }
                case IDC_ADDSECTION:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    int tj = is_Section_First(acFrame);
                    char tbuf[SIZE_SECTION_NAMES];
                    GetDlgItemTextA(hwTB, IDC_SECTIONNAME, tbuf, 31);
                    tbuf[31] = 0;
                    int tnampos = Duplicate_Section_Name(tbuf);
                    if (tnampos > -1)
                    {
                        MessageBoxA(hWnd, "This name is already used for another section, action ignored.", "Caution", MB_OK);
                        return TRUE;
                    }
                    SaveAction(true, SA_SECTIONS);
                    if (tj > -1)
                    {
                        strcpy_s(&MycRP.Section_Names[SIZE_SECTION_NAMES * tj], SIZE_SECTION_NAMES, tbuf);
                    }
                    else
                    {
                        MycRP.Section_Firsts[MycRP.nSections] = acFrame;
                        strcpy_s(&MycRP.Section_Names[SIZE_SECTION_NAMES * MycRP.nSections], SIZE_SECTION_NAMES, tbuf);
                        MycRP.nSections++;
                    }
                    UpdateSectionList();
                    UpdateFSneeded = true;
                    return TRUE;
                }
                case IDC_DELSECTION:
                {
                    int ti = Which_Section(acFrame);
                    if (ti == -1) return TRUE;
                    char tbuf[256];
                    sprintf_s(tbuf, 256, "Confirm you want to delete the section \"%s\" this frame is part of?", &MycRP.Section_Names[ti * SIZE_SECTION_NAMES]);
                    if (MessageBoxA(hwTB, tbuf, "Confirm", MB_YESNO) == IDYES)
                    {
                        SaveAction(true, SA_SECTIONS);
                        Delete_Section(ti);
                        UpdateSectionList();
                    }
                    UpdateFSneeded = true;
                    return TRUE;
                }
                case IDC_MOVESECTION:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    RECT rc;
                    GetWindowRect(GetDlgItem(hwTB, IDC_MOVESECTION), &rc);
                    hMovSec = CreateWindowEx(0, L"MovSection", L"", WS_POPUP, rc.left-50, rc.top, 300, 600, NULL, NULL, hInst, NULL);       // Parent window.
                    if (!hMovSec)
                    {
                        AffLastError((char*)"Create Move Section Window");
                        return TRUE;
                    }
                    ShowWindow(hMovSec, true);
                    SaveAction(true, SA_SECTIONS);
                    return TRUE;
                }
                case IDC_SECTIONLIST:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    unsigned char acpos = (unsigned char)SendMessage(GetDlgItem(hDlg, IDC_SECTIONLIST), CB_GETCURSEL, 0, 0) - 1;
                    if ((HIWORD(wParam) == CBN_SELCHANGE) && (acpos < MAX_SECTIONS - 1))
                    {
                        PreFrameInStrip = MycRP.Section_Firsts[acpos];
                        UpdateFSneeded = true;
                        SetDlgItemTextA(hwTB, IDC_SECTIONNAME, "");// &MycRP.Section_Names[acpos * SIZE_SECTION_NAMES]);
                    }
                    return TRUE;
                }
                case IDC_COLTODYNA:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SaveAction(true,SA_DYNACOLOR);
                    for (UINT tj = 0; tj < nSelFrames; tj++)
                    {
                        for (UINT ti = 0; ti < MycRom.noColors; ti++)
                        {
                            MycRom.Dyna4Cols[(SelFrames[tj] * MAX_DYNA_SETS_PER_FRAME + acDynaSet) * MycRom.noColors + ti] = acEditColors[ti];
                        }
                    }
                    for (UINT ti = IDC_DYNACOL1; ti <= IDC_DYNACOL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
                    return TRUE;
                }
                case IDC_INVERTSEL:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SaveAction(true, SA_COMPMASK);
                    UINT nomsk = (UINT)MycRom.CompMaskID[acFrame];
                    if (nomsk < 255)
                    {
                        nomsk *= MycRom.fWidth * MycRom.fHeight;
                        for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
                        {
                            if (MycRom.CompMasks[nomsk + ti] == 0) MycRom.CompMasks[nomsk + ti] = 1; else MycRom.CompMasks[nomsk + ti] = 0;
                        }
                    }
                    CheckSameFrames();
                    return TRUE;
                }
                case IDC_INVERTSEL2:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SaveAction(true, SA_COPYMASK);
                    for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
                    {
                        // the whole frame has been copied to col and dyna, so we just invert the mask
                        if (Copy_Mask[ti] == 0) Copy_Mask[ti] = 1; else Copy_Mask[ti] = 0;
                    }
                    return TRUE;
                }
                case IDC_ADDSPRITE2:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (MycRom.nSprites == 0) return TRUE;
                    for (UINT ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
                    {
                        if (MycRom.FrameSprites[acFrame * MAX_SPRITES_PER_FRAME + ti] == acSprite) return TRUE;
                    }
                    UINT ti = 0;
                    while ((MycRom.FrameSprites[acFrame * MAX_SPRITES_PER_FRAME + ti] != 255) && (ti < MAX_SPRITES_PER_FRAME)) ti++;
                    if (ti == MAX_SPRITES_PER_FRAME)
                    {
                        MessageBoxA(hwTB2, "This frame has already reached the maximum number of sprites.", "Failed", MB_OK);
                        return TRUE;
                    }
                    SaveAction(true, SA_FRAMESPRITES);
                    MycRom.FrameSprites[acFrame * MAX_SPRITES_PER_FRAME + ti] = acSprite;
                    UpdateFrameSpriteList();
                    UpdateSpriteList3();
                    return TRUE;
                }
                case IDC_DELSPRITE2:
                {
                    SaveAction(true, SA_FRAMESPRITES);
                    int acpos = (int)SendMessage(GetDlgItem(hwTB, IDC_SPRITELIST2), CB_GETCURSEL, 0, 0);
                    if (acpos == -1) return TRUE;
                    SaveAction(true, SA_FRAMESPRITES);
                    for (UINT ti = acpos; ti < MAX_SPRITES_PER_FRAME - 1; ti++) MycRom.FrameSprites[acFrame * MAX_SPRITES_PER_FRAME + ti] = MycRom.FrameSprites[acFrame * MAX_SPRITES_PER_FRAME + ti + 1];
                    UpdateFrameSpriteList();
                    UpdateSpriteList3();
                    return TRUE;
                }
                case IDC_COLROT:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    SaveAction(true, SA_FRAMESPRITES);
                    if (hPal)
                    {
                        DestroyWindow(hPal);
                        hPal = NULL;
                    }
                    if (hColSet) DestroyWindow(hColSet);
                    ColSetMode = 1;
                    hColSet = CreateDialog(hInst, MAKEINTRESOURCE(IDD_COLSET), hDlg, (DLGPROC)ColSet_Proc);
                    if (!hColSet)
                    {
                        AffLastError((char*)"ColSet dialog display");
                        break;
                    }
                    RECT rc;
                    GetWindowRect(GetDlgItem(hwTB, IDC_COLROT), &rc);
                    SetWindowPos(hColSet, 0, rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                    ShowWindow(hColSet, TRUE);
                    return TRUE;
                }
                case IDC_FRUSEMASK:
                {
                    if (HIWORD(wParam) == LBN_SELCHANGE)
                    {
                        int acpos = (int)SendMessage((HWND)lParam, LB_GETCURSEL, 0, 0);
                        if (acpos >= 0)
                        {
                            char tbuf[16];
                            SendMessageA((HWND)lParam, LB_GETTEXT, acpos, (LPARAM)tbuf);
                            PreFrameInStrip = atoi(tbuf);
                            UpdateFSneeded = true;
                        }
                    }
                    return TRUE;
                }
                case IDC_IDENT:
                {
                    if (Ident_Pushed) Ident_Pushed = false;
                    else Ident_Pushed = true;
                    SetSpotButton(Ident_Pushed);
                    return TRUE;
                }
                case IDC_DELTID:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    MycRom.TriggerID[acFrame] = 0xFFFFFFFF;
                    UpdateTriggerID();
                    return TRUE;
                }
                case IDC_TRIGID:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        char tbuf[256];
                        GetWindowTextA((HWND)lParam, tbuf, 256);
                        if (strcmp(tbuf, "- None -") == 0) return TRUE;
                        int tID = atoi(tbuf);
                        if (tID < 0)
                        {
                            tID = 0;
                            strcpy_s(tbuf, 256, "0");
                            SetWindowTextA((HWND)lParam, tbuf);
                        }
                        if (tID >= 65536)
                        {
                            tID = 65535;
                            strcpy_s(tbuf, 256, "65535");
                            SetWindowTextA((HWND)lParam, tbuf);
                        }
                        /*for (UINT tj = 0; tj < MycRom.nFrames; tj++)
                        {
                            if (tID == MycRom.TriggerID[tj])
                        }*/
                        MycRom.TriggerID[acFrame] = tID;
                    }
                    return TRUE;
                }
                case IDC_NIGHTDAY:
                {
                    HBRUSH brush;
                    if (!Night_Mode)
                    {
                        brush = CreateSolidBrush(RGB(20, 20, 20));
                        Night_Mode = true;
                    }
                    else
                    {
                        brush = CreateSolidBrush(RGB(240, 240, 240));
                        Night_Mode = false;
                    }
                    SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
                    SetClassLongPtr(hwTB, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
                    SetClassLongPtr(hSprites, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
                    SetClassLongPtr(hwTB2, GCLP_HBRBACKGROUND, (LONG_PTR)brush);
                    InvalidateRect(hWnd, NULL, TRUE);
                    InvalidateRect(hwTB, NULL, TRUE);
                    InvalidateRect(hSprites, NULL, TRUE);
                    InvalidateRect(hwTB2, NULL, TRUE);
                    return TRUE;
                }
                /*case IDC_ADDTID:
                {
                    if (MycRom.name[0] == 0) return TRUE;
                    if (MycRom.TriggerID[acFrame] != 0xFFFFFFFF) return TRUE;
                    UINT32 ti = 0;
                    bool tidfound = false;
                    while (!tidfound)
                    {
                        UINT32 tj;
                        for (tj = 0; tj < MycRom.nFrames; tj++)
                        {
                            if (ti == MycRom.TriggerID[tj]) break;
                        }
                        if (tj == MycRom.nFrames) tidfound = true;
                        else ti++;
                    }
                    MycRom.TriggerID[acFrame] = ti;
                    UpdateTriggerID();
                    return TRUE;
                }*/
            }
            break;
        }
        case WM_SETCURSOR:
        {
            if (Paste_Mode) SetCursor(hcPaste);
            else if (Color_Pipette) SetCursor(hcColPick);
            else DefWindowProc(hDlg, message, wParam, lParam);
            return TRUE;
        }
        case WM_NOTIFY:
        {
            UINT nCode = ((LPNMHDR)lParam)->code;

            switch (nCode)
            {
                case UDN_DELTAPOS:
                {
                    LPNMUPDOWN lpnmud = (LPNMUPDOWN)lParam;
                    if (lpnmud->hdr.hwndFrom == GetDlgItem(hwTB, IDC_CHANGECOLSET))
                    {
                        if ((lpnmud->iDelta < 0) && (acDynaSet > 0)) acDynaSet--;
                        else if ((lpnmud->iDelta > 0) && (acDynaSet < MAX_DYNA_SETS_PER_FRAME - 1)) acDynaSet++;
                        char tbuf[10];
                        _itoa_s(acDynaSet + 1, tbuf, 8, 10);
                        SetDlgItemTextA(hwTB, IDC_NOSETCOL, tbuf);
                        for (UINT ti = IDC_DYNACOL1; ti <= IDC_DYNACOL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
                        return TRUE;
                    }
                    break;
                }
            }
        }
    }
    return (INT_PTR)FALSE;
}


const char* ButtonDescription2(HWND hOver)
{
    if (hOver == GetDlgItem(hwTB2, IDC_SAVE)) return (const char*)"Save the Serum project";
    for (int ti = 0; ti < 16; ti++)
    {
        if (hOver == GetDlgItem(hwTB2, IDC_COL1 + ti))
        {
            char tbuf[512];
            sprintf_s(tbuf, 512, "Select the color #%i for drawing in the 64-colour palette", ti + 1);
            return tbuf;
        }
    }
    if (hOver == GetDlgItem(hwTB2, IDC_SPRITENAME)) return (const char*)"Enter the name for a new sprite here";
    if (hOver == GetDlgItem(hwTB2, IDC_SPRITELIST)) return (const char*)"List of all the sprites";
    if (hOver == GetDlgItem(hwTB2, IDC_ADDSPRITE)) return (const char*)"Add a sprite named after the text in the box beside";
    if (hOver == GetDlgItem(hwTB2, IDC_DELSPRITE)) return (const char*)"Delete the sprite";
    if (hOver == GetDlgItem(hwTB2, IDC_UNDO)) return (const char*)"Undo last action";
    if (hOver == GetDlgItem(hwTB2, IDC_REDO)) return (const char*)"Redo last action";
    if (hOver == GetDlgItem(hwTB2, IDC_PASTE)) return (const char*)"Paste the selection from the frame as the selected sprite";
    if (hOver == GetDlgItem(hwTB2, IDC_DRAWPOINT)) return (const char*)"Point tool to draw the sprite or the detection area";
    if (hOver == GetDlgItem(hwTB2, IDC_DRAWLINE)) return (const char*)"Line tool to draw the sprite or the detection area";
    if (hOver == GetDlgItem(hwTB2, IDC_DRAWRECT)) return (const char*)"Rectangle tool to draw the sprite or the detection area";
    if (hOver == GetDlgItem(hwTB2, IDC_DRAWCIRC)) return (const char*)"Circle tool to draw the sprite or the detection area";
    if (hOver == GetDlgItem(hwTB2, IDC_FILL)) return (const char*)"Magic wand tool to draw the sprite or the detection area";
    if (hOver == GetDlgItem(hwTB2, IDC_FILLED)) return (const char*)"Draw rectangles or circles are filled or not?";
    if (hOver == GetDlgItem(hwTB2, IDC_ADDTOFRAME)) return (const char*)"Add this sprite to the list of sprites to be detected to the current displayed frame";
    if (hOver == GetDlgItem(hwTB2, IDC_TOFRAME)) return (const char*)"Display the frame this sprite has been extracted from in the colorization window (will be wrong if this frame has been deleted)";
    if (hOver == GetDlgItem(hwTB2, IDC_DETSPR)) return (const char*)"Select the detection zone to draw";
    if (hOver == GetDlgItem(hwTB2, IDC_DELDETSPR)) return (const char*)"Delete this detection zone from the sprite";
    if (hOver == GetDlgItem(hwTB2, IDC_SPRITELIST2)) return (const char*)"Select a sprite to show the frames using it below";
    if (hOver == GetDlgItem(hwTB2, IDC_FRUSESPRITE)) return (const char*)"List of frames using the sprite selected above";
    return "";
}

LRESULT CALLBACK ButtonSubclassProc2(HWND hBut, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    switch (message) {
        case WM_KEYDOWN:
        {
            return TRUE;
        }
        case WM_MOUSEMOVE:
        {
            TRACKMOUSEEVENT tme;
            tme.cbSize = sizeof(TRACKMOUSEEVENT);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hBut;
            tme.dwHoverTime = HOVER_DEFAULT;
            TrackMouseEvent(&tme);
            SetWindowTextA(hStatus2, ButtonDescription2(hBut));
            break;
        }
        case WM_MOUSELEAVE:
        {
            // The mouse has left the button
            SetWindowTextA(hStatus2, "");
            break;
        }
    }

    // Call the default button window procedure for any unhandled messages
    return DefSubclassProc(hBut, message, wParam, lParam);
}

INT_PTR CALLBACK Toolbar_Proc2(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        SetWindowSubclass(GetDlgItem(hDlg, IDC_SAVE), ButtonSubclassProc2, 0, 0);
        for (int ti = 0; ti < 16; ti++) SetWindowSubclass(GetDlgItem(hDlg, IDC_COL1 + ti), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_SPRITENAME), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_SPRITELIST), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_ADDSPRITE), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_DELSPRITE), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_UNDO), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_REDO), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_PASTE), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWPOINT), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWLINE), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWRECT), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_DRAWCIRC), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_FILL), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_FILLED), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_ADDTOFRAME), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_TOFRAME), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_DETSPR), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_DELDETSPR), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_SPRITELIST2), ButtonSubclassProc2, 0, 0);
        SetWindowSubclass(GetDlgItem(hDlg, IDC_FRUSESPRITE), ButtonSubclassProc2, 0, 0);
    }
    case WM_CTLCOLORDLG:
    {
        if (Night_Mode) return (INT_PTR)GetStockObject(DKGRAY_BRUSH);
        return (INT_PTR)GetStockObject(GRAY_BRUSH);
    }
    case WM_PAINT:
    {
        if (MycRom.name[0] == 0) return FALSE;
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hDlg, &ps);
        RECT rc;
        GetWindowRect(GetDlgItem(hDlg, IDC_DRAWPOINT + Sprite_Mode), &rc);
        rc.bottom += 5;
        HBRUSH br = CreateSolidBrush(RGB(255, 0, 0));
        ScreenToClient(hDlg, &rc);
        FillRect(hdc, &rc, br);
        DeleteObject(br);
        EndPaint(hDlg, &ps);
        return TRUE;
    }
    case WM_DRAWITEM:
    {
        if ((MycRom.name[0] == 0) || (MycRom.nSprites == 0)) return TRUE;
        LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;
        UINT tpm;
        if ((lpdis->CtlID >= IDC_COL1) && (lpdis->CtlID <= IDC_COL16))
        {
            if (lpdis->CtlID) tpm = lpdis->CtlID - IDC_COL1;
            HBRUSH bg = CreateSolidBrush(RGB(MycRom.cPal[MycRP.Sprite_Col_From_Frame[acSprite] * 3 * 64 + MycRP.Sprite_Edit_Colors[acSprite * 16 + tpm] * 3], MycRom.cPal[MycRP.Sprite_Col_From_Frame[acSprite] * 3 * 64 + MycRP.Sprite_Edit_Colors[acSprite * 16 + tpm] * 3 + 1], MycRom.cPal[MycRP.Sprite_Col_From_Frame[acSprite] * 3 * 64 + MycRP.Sprite_Edit_Colors[acSprite * 16 + tpm] * 3 + 2]));
            RECT rc = lpdis->rcItem;
            if (tpm == noSprSel)
            {
                HBRUSH br = CreateSolidBrush(RGB(255, 0, 0));
                FillRect(lpdis->hDC, &rc, br);
                DeleteObject(br);
            }
            rc.left += 2;
            rc.right -= 2;
            rc.top += 2;
            rc.bottom -= 2;
            FillRect(lpdis->hDC, &rc, bg);
            DeleteObject(bg);
        }
        return TRUE;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
            case IDC_ADDSPRITE:
            {
                if (MycRom.name[0] == 0) return TRUE;
                if (MycRom.nSprites == 255) return TRUE;
                char tbuf[SIZE_SECTION_NAMES];
                GetDlgItemTextA(hwTB2, IDC_SPRITENAME, tbuf, SIZE_SECTION_NAMES-1);
                if (tbuf[0] == 0) return TRUE;
                tbuf[SIZE_SECTION_NAMES-1] = 0;
                int tnampos = Duplicate_Sprite_Name(tbuf);
                if (tnampos > -1)
                {
                    MessageBoxA(hSprites, "This name is already used for another sprite, action ignored.", "Caution", MB_OK);
                    return TRUE;
                }
                SaveAction(true, SA_SPRITES);
                MycRP.Sprite_Col_From_Frame[MycRom.nSprites] = acFrame;
                strcpy_s(&MycRP.Sprite_Names[SIZE_SECTION_NAMES * MycRom.nSprites], SIZE_SECTION_NAMES, tbuf);
                MycRom.SpriteDescriptions = (UINT16*)realloc(MycRom.SpriteDescriptions, sizeof(UINT16) * (MycRom.nSprites + 1) * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE);
                memset(&MycRom.SpriteDescriptions[MycRom.nSprites * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE], 255, MAX_SPRITE_SIZE * MAX_SPRITE_SIZE * sizeof(UINT16));
                for (UINT ti = 0; ti < 16; ti++) MycRP.Sprite_Edit_Colors[acSprite * 16 + ti] = ti;
                MycRom.SpriteDetDwords = (UINT32*)realloc(MycRom.SpriteDetDwords, sizeof(UINT32) * (MycRom.nSprites + 1)*MAX_SPRITE_DETECT_AREAS);
                MycRom.SpriteDetDwordPos = (UINT16*)realloc(MycRom.SpriteDetDwordPos, sizeof(UINT16) * (MycRom.nSprites + 1) * MAX_SPRITE_DETECT_AREAS);
                MycRom.SpriteDetAreas = (UINT16*)realloc(MycRom.SpriteDetAreas, sizeof(UINT16) * (MycRom.nSprites + 1) * MAX_SPRITE_DETECT_AREAS*4);
                for (UINT ti = 0; ti < MAX_SPRITE_DETECT_AREAS; ti++) MycRom.SpriteDetAreas[MycRom.nSprites * 4 * MAX_SPRITE_DETECT_AREAS + ti * 4] = 0xffff; // no dword defined
                SendMessage(GetDlgItem(hwTB2, IDC_ISDWORD), BM_SETCHECK, BST_UNCHECKED, 0);
                acSprite = MycRom.nSprites;
                MycRom.nSprites++;
                for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++) InvalidateRect(GetDlgItem(hwTB2, ti), NULL, TRUE);
                UpdateSpriteList();
                if (acSprite >= PreSpriteInStrip + NSpriteToDraw) PreSpriteInStrip = acSprite - NSpriteToDraw + 1;
                if ((int)acSprite < PreSpriteInStrip) PreSpriteInStrip = acSprite;
                UpdateSSneeded = true;
                return TRUE;
            }
            case IDC_DELSPRITE:
            {
                if (MycRom.nSprites == 0) return TRUE;
                char tbuf[256];
                sprintf_s(tbuf, 256, "Confirm you want to delete the sprite %s, this action can not be undone ?", &MycRP.Sprite_Names[acSprite * SIZE_SECTION_NAMES]);
                if (MessageBoxA(hSprites, tbuf, "Confirm", MB_YESNO) == IDYES)
                {
                    SaveAction(true, SA_SPRITES);
                    for (UINT ti = 0; ti < MycRom.nFrames; ti++)
                    {
                        for (UINT tj = 0; tj < MAX_SPRITES_PER_FRAME; tj++)
                        {
                            if (MycRom.FrameSprites[ti * MAX_SPRITES_PER_FRAME + tj] == acSprite) MycRom.FrameSprites[ti * MAX_SPRITES_PER_FRAME + tj] = 255;
                            if ((MycRom.FrameSprites[ti * MAX_SPRITES_PER_FRAME + tj] > acSprite) && (MycRom.FrameSprites[ti * MAX_SPRITES_PER_FRAME + tj] != 255)) MycRom.FrameSprites[ti * MAX_SPRITES_PER_FRAME + tj]--;
                        }
                    }
                    Delete_Sprite(acSprite);
                    UpdateSpriteList();
                    UpdateFrameSpriteList();
                    for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++) InvalidateRect(GetDlgItem(hwTB2, ti), NULL, TRUE);
                }
                if ((acSprite >= MycRom.nSprites) && (MycRom.nSprites > 0)) acSprite = MycRom.nSprites - 1;
                if (acSprite >= PreSpriteInStrip + NSpriteToDraw) PreSpriteInStrip = acSprite - NSpriteToDraw + 1;
                if ((int)acSprite < PreSpriteInStrip) PreSpriteInStrip = acSprite;
                UpdateSSneeded = true;
                return TRUE;
            }
            case IDC_SPRITELIST:
            {
                if (MycRom.name[0] == 0) return TRUE;
                if (HIWORD(wParam) == CBN_SELCHANGE)
                {
                    SaveAction(true, SA_ACSPRITE);
                    acSprite = (UINT)SendMessage(GetDlgItem(hDlg, IDC_SPRITELIST), CB_GETCURSEL, 0, 0);
                    for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++) InvalidateRect(GetDlgItem(hwTB2, ti), NULL, TRUE);
                    //SetDlgItemTextA(hwTB, IDC_SPRITENAME, &MycRP.Sprite_Names[acSprite * SIZE_SECTION_NAMES]);
                    if (acSprite >= PreSpriteInStrip + NSpriteToDraw) PreSpriteInStrip = acSprite - NSpriteToDraw + 1;
                    if ((int)acSprite < PreSpriteInStrip) PreSpriteInStrip = acSprite;
                    UpdateSSneeded = true;
                }
                return TRUE;
            }
            case IDC_ADDTOFRAME:
            {
                if (MycRom.name[0] == 0) return TRUE;
                SaveAction(true, SA_FRAMESPRITES);
                for (UINT tj = 0; tj < nSelFrames; tj++)
                {
                    for (UINT ti = 0; ti < MAX_SPRITES_PER_FRAME; ti++)
                    {
                        if (MycRom.FrameSprites[SelFrames[tj] * MAX_SPRITES_PER_FRAME + ti] == acSprite) return TRUE;
                    }
                    UINT ti = 0;
                    while ((MycRom.FrameSprites[SelFrames[tj] * MAX_SPRITES_PER_FRAME + ti] != 255) && (ti < MAX_SPRITES_PER_FRAME)) ti++;
                    if (ti == MAX_SPRITES_PER_FRAME)
                    {
                        MessageBoxA(hwTB2, "One of the selected frames has already reach the maximum number of sprites.", "Failed", MB_OK);
                        return TRUE;
                    }
                    MycRom.FrameSprites[SelFrames[tj] * MAX_SPRITES_PER_FRAME + ti] = acSprite;
                }
                UpdateFrameSpriteList();
                UpdateSpriteList3();
                return TRUE;
            }
            case IDC_PASTE:
            {
                if (MycRom.name[0] == 0) return TRUE;
                if (MycRom.nSprites == 0) return TRUE;
                UpdateSSneeded = true;
                MycRP.Sprite_Col_From_Frame[acSprite] = acFrame;
                SaveAction(true, SA_SPRITE);
                UINT8 minx_cpy = 255, maxx_cpy = 0, miny_cpy = 63, maxy_cpy = 0;
                for (UINT tj = 0; tj < MycRom.fHeight; tj++)
                {
                    for (UINT ti = 0; ti < MycRom.fWidth; ti++)
                    {
                        if (Copy_Mask[tj * MycRom.fWidth + ti] > 0)
                        {
                            if (tj > maxy_cpy) maxy_cpy = tj;
                            if (tj < miny_cpy) miny_cpy = tj;
                            if (ti > maxx_cpy) maxx_cpy = ti;
                            if (ti < minx_cpy) minx_cpy = ti;
                        }
                    }
                }
                MycRP.SpriteRect[acSprite * 4] = minx_cpy;
                MycRP.SpriteRect[acSprite * 4 + 1] = miny_cpy;
                MycRP.SpriteRect[acSprite * 4 + 2] = maxx_cpy;
                MycRP.SpriteRect[acSprite * 4 + 3] = maxy_cpy;
                if (maxy_cpy - miny_cpy >= MAX_SPRITE_SIZE) maxy_cpy = miny_cpy + MAX_SPRITE_SIZE - 1;
                if (maxx_cpy - minx_cpy >= MAX_SPRITE_SIZE) maxx_cpy = minx_cpy + MAX_SPRITE_SIZE - 1;
                for (UINT tj = miny_cpy; tj <= maxy_cpy; tj++)
                {
                    for (UINT ti = minx_cpy; ti <= maxx_cpy; ti++)
                    {
                        int i = ti, j = tj;
                        MycRP.SpriteRectMirror[acSprite * 2] = FALSE;
                        MycRP.SpriteRectMirror[acSprite * 2 + 1] = FALSE;
                        if (GetKeyState('K') & 0x8000)
                        {
                            i = minx_cpy + maxx_cpy - ti;
                            MycRP.SpriteRectMirror[acSprite * 2] = TRUE;
                        }
                        if (GetKeyState('L') & 0x8000)
                        {
                            j = miny_cpy + maxy_cpy - tj;
                            MycRP.SpriteRectMirror[acSprite * 2 + 1] = TRUE;
                        }
                        if (Copy_Mask[j * MycRom.fWidth + i] > 0)
                            MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + (tj - miny_cpy) * MAX_SPRITE_SIZE + (ti - minx_cpy)] = ((UINT16)Copy_Colo[j * MycRom.fWidth + i] << 8) +
                                (UINT16)Copy_Col[j * MycRom.fWidth + i];
                        else
                            MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + (tj - miny_cpy) * MAX_SPRITE_SIZE + (ti - minx_cpy)] = ((UINT16)255) << 8;
                    }
                }
                return TRUE;
            }
            case IDC_UNDO:
            {
                if (MycRom.name[0] == 0) return TRUE;
                RecoverAction(true);
                return TRUE;
            }
            case IDC_REDO:
            {
                if (MycRom.name[0] == 0) return TRUE;
                RecoverAction(false);
                return TRUE;
            }
            case IDC_COL1:
            case IDC_COL2:
            case IDC_COL3:
            case IDC_COL4:
            case IDC_COL5:
            case IDC_COL6:
            case IDC_COL7:
            case IDC_COL8:
            case IDC_COL9:
            case IDC_COL10:
            case IDC_COL11:
            case IDC_COL12:
            case IDC_COL13:
            case IDC_COL14:
            case IDC_COL15:
            case IDC_COL16:
            {
                noSprSel = LOWORD(wParam) - IDC_COL1;
                if (MycRom.name[0] == 0) return TRUE;
                if (hColSet)
                {
                    DestroyWindow(hColSet);
                    hColSet = NULL;
                }
                Choose_Color_Palette2(LOWORD(wParam));
                InvalidateRect(hwTB2, NULL, TRUE);
                return TRUE;
            }
            case IDC_DRAWPOINT:
            case IDC_DRAWLINE:
            case IDC_DRAWRECT:
            case IDC_DRAWCIRC:
            case IDC_FILL:
            {
                Sprite_Mode = (UINT8)(LOWORD(wParam) - IDC_DRAWPOINT);
                InvalidateRect(hDlg, NULL, TRUE);
                return TRUE;
            }
            case IDC_FILLED:
            {
                if (Button_GetCheck(GetDlgItem(hwTB2, IDC_FILLED)) == BST_CHECKED) SpriteFill_Mode = TRUE; else SpriteFill_Mode = FALSE;
                return TRUE;
            }
            case IDC_SAVE:
            {
                if (MycRom.name[0] == 0) return TRUE;
                if (Save_cRom(false))
                {
                    cprintf("%s.cROM saved in %s", MycRom.name, MycRP.SaveDir);
                    if (Save_cRP(false)) cprintf("%s.cRP saved in %s", MycRom.name, MycRP.SaveDir);
                }
                return TRUE;
            }
            case IDC_DETSPR:
            {
                if (MycRom.name[0] == 0) return TRUE;
                acDetSprite = (UINT)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                return TRUE;
            }
            case IDC_DELDETSPR:
            {
                if (MycRom.name[0] == 0) return TRUE;
                if (MycRom.nSprites == 0) return TRUE;
                SaveAction(true, SA_SPRITE);
                MycRom.SpriteDetAreas[acSprite * 4 * MAX_SPRITE_DETECT_AREAS + acDetSprite * 4] = 255;
                return TRUE;
            }
            case IDC_TOFRAME:
            {
                if (MycRom.name[0] == 0) return TRUE;
                if (MycRom.nSprites == 0) return TRUE;
                acFrame = MycRP.Sprite_Col_From_Frame[acSprite];
                if (acFrame >= MycRom.nFrames) acFrame = MycRom.nFrames - 1;
                if (acFrame >= PreFrameInStrip + NFrameToDraw) PreFrameInStrip = acFrame - NFrameToDraw + 1;
                if ((int)acFrame < PreFrameInStrip) PreFrameInStrip = acFrame;
                UpdateFSneeded = true;
                UpdateNewacFrame();
                if (MycRP.SpriteRect[4 * acSprite] < 0xffff)
                {
                    memset(Copy_Mask, 0, MycRom.fWidth * MycRom.fHeight);
                    for (UINT tj = MycRP.SpriteRect[4 * acSprite + 1]; tj <= MycRP.SpriteRect[4 * acSprite + 3]; tj++)
                    {
                        for (UINT ti = MycRP.SpriteRect[4 * acSprite]; ti <= MycRP.SpriteRect[4 * acSprite + 2]; ti++)
                        {
                            UINT i = ti, j = tj;
                            if (MycRP.SpriteRectMirror[2 * acSprite] == TRUE) i = MycRP.SpriteRect[4 * acSprite + 2] - (ti - MycRP.SpriteRect[4 * acSprite]);
                            if (MycRP.SpriteRectMirror[2 * acSprite + 1] == TRUE) j = MycRP.SpriteRect[4 * acSprite + 3] - (tj - MycRP.SpriteRect[4 * acSprite + 1]);
                            if ((MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + (tj - MycRP.SpriteRect[4 * acSprite + 1]) * MAX_SPRITE_SIZE + ti - MycRP.SpriteRect[4 * acSprite]] & 0x8000) == 0)
                            {
                                Copy_Mask[i + j * MycRom.fWidth] = 1;
                            }
                        }
                    }
                }
                return TRUE;
            }
            case IDC_SPRITELIST2:
            {
                UpdateSpriteList2();
                return TRUE;
            }
        }
        return (INT_PTR)FALSE;
    }
    }
    return (INT_PTR)FALSE;
}

bool isSReleased = false, isEnterReleased = false, isZReleased = false, isYReleased = false, isMReleased = false, isAReleased = false, isCReleased = false, isVReleased = false, isFReleased = false, isDReleased = false, isEReleased = false;
bool is4Released = false, is6Released = false, is2Released = false, is8Released = false;
DWORD NumPadNextTime = 0;
void CheckAccelerators(void)
{
    if (!(GetKeyState('S') & 0x8000)) isSReleased = true;
    if (!(GetKeyState('Z') & 0x8000)) isZReleased = true;
    if (!(GetKeyState('Y') & 0x8000)) isYReleased = true;
    if (!(GetKeyState('M') & 0x8000)) isMReleased = true;
    if (!(GetKeyState('F') & 0x8000)) isFReleased = true;
    if (!(GetKeyState('C') & 0x8000)) isCReleased = true;
    if (!(GetKeyState('A') & 0x8000)) isAReleased = true;
    if (!(GetKeyState('D') & 0x8000)) isDReleased = true;
    if (!(GetKeyState('E') & 0x8000)) isEReleased = true;
    if (!(GetKeyState('V') & 0x8000)) isVReleased = true;
    if (!(GetKeyState(VK_NUMPAD2) & 0x8000)) is2Released = true;
    if (!(GetKeyState(VK_NUMPAD4) & 0x8000)) is4Released = true;
    if (!(GetKeyState(VK_NUMPAD6) & 0x8000)) is6Released = true;
    if (!(GetKeyState(VK_NUMPAD8) & 0x8000)) is8Released = true;
    if (!(GetKeyState(VK_RETURN) & 0x8000)) isEnterReleased = true;
    if (GetForegroundWindow() == hWnd)
    {
        if (MycRom.name[0])
        {
            if (Paste_Mode)
            {
                if (GetKeyState(VK_NUMPAD2) & 0x8000)
                {
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime)
                    {
                        if (is2Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is2Released = false;
                        paste_offsety += 1;
                    }
                }
                if (GetKeyState(VK_NUMPAD4) & 0x8000)
                {
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime)
                    {
                        if (is4Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is4Released = false;
                        paste_offsetx -= 1;
                    }
                }
                if (GetKeyState(VK_NUMPAD6) & 0x8000)
                {
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime)
                    {
                        if (is6Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is6Released = false;
                        paste_offsetx += 1;
                    }
                }
                if (GetKeyState(VK_NUMPAD8) & 0x8000)
                {
                    DWORD newtime = timeGetTime();
                    if (newtime >= NumPadNextTime)
                    {
                        if (is8Released) NumPadNextTime = newtime + 500;
                        else NumPadNextTime = newtime + 50;
                        is8Released = false;
                        paste_offsety -= 1;
                    }
                }
            }
            if ((GetKeyState(VK_CONTROL) & 0x8000) && (!Paste_Mode) && (!Color_Pipette))
            {
                if ((isSReleased) && (GetKeyState('S') & 0x8000))
                {
                    isSReleased = false;
                    Save_cRP(false);
                    Save_cRom(false);
                }
                if ((isZReleased) && (GetKeyState('Z') & 0x8000))
                {
                    isZReleased = false;
                    RecoverAction(true);
                }
                if ((isYReleased) && (GetKeyState('Y') & 0x8000))
                {
                    isYReleased = false;
                    RecoverAction(false);
                }
                if ((isMReleased) && (GetKeyState('M') & 0x8000))
                {
                    isMReleased = false;
                    Edit_Mode = !Edit_Mode;
                    Update_Toolbar = true;// CreateToolbar();
                    UpdateFSneeded = true;
                }
                if ((isAReleased) && (GetKeyState('A') & 0x8000))
                {
                    if (Edit_Mode == 0)
                    {
                        if (MycRom.CompMaskID[acFrame] != 255)
                        {
                            SaveAction(true, SA_COMPMASK);
                            memset(&MycRom.CompMasks[MycRom.CompMaskID[acFrame] * MycRom.fWidth * MycRom.fHeight], 1, MycRom.fWidth * MycRom.fHeight);
                            CheckSameFrames();
                        }
                    }
                    else
                    {
                        SaveAction(true, SA_COPYMASK);
                        memset(Draw_Extra_Surface, 1, 256 * 64);
                        Add_Surface_To_Copy(Draw_Extra_Surface, false);
                    }
                    isAReleased = false;
                }
                if ((isDReleased) && (GetKeyState('D') & 0x8000))
                {
                    if (Edit_Mode == 0)
                    {
                        if (MycRom.CompMaskID[acFrame] != 255)
                        {
                            SaveAction(true, SA_COMPMASK);
                            memset(&MycRom.CompMasks[MycRom.CompMaskID[acFrame] * MycRom.fWidth * MycRom.fHeight], 0, MycRom.fWidth * MycRom.fHeight);
                            CheckSameFrames();
                        }
                    }
                    else
                    {
                        SaveAction(true, SA_COPYMASK);
                        memset(Draw_Extra_Surface, 1, 256 * 64);
                        Add_Surface_To_Copy(Draw_Extra_Surface, true);
                    }
                    isDReleased = false;
                }
                if ((isFReleased) && (GetKeyState('F') & 0x8000))
                {
                    isFReleased = false;
                    PreFrameInStrip = acFrame;
                    UpdateFSneeded = true;
                }
                if (Edit_Mode == 1)
                {
                    if ((isCReleased) && (GetKeyState('C') & 0x8000))
                    {
                        isCReleased = false;
                        SendMessage(hwTB, WM_COMMAND, IDC_COPY, 0);
                    }
                    if ((isVReleased) && (GetKeyState('V') & 0x8000))
                    {
                        isVReleased = false;
                        if (IsWindowEnabled(GetDlgItem(hwTB, IDC_PASTE)) == TRUE) SendMessage(hwTB, WM_COMMAND, IDC_PASTE, 0);
                    }
                    if ((isEReleased) && (GetKeyState('E') & 0x8000))
                    {
                        isEReleased = false;
                        SaveAction(true, SA_DYNAMASK);
                        for (UINT ti = 0; ti < nSelFrames; ti++)
                        {
                            for (UINT tj = 0; tj < MycRom.fWidth * MycRom.fHeight; tj++)
                            {
                                if (MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight+tj]<255)
                                MycRom.cFrames[SelFrames[ti] * MycRom.fHeight * MycRom.fWidth + tj] = MycRP.oFrames[SelFrames[ti] * MycRom.fHeight * MycRom.fWidth + tj];
                            }
                            memset(&MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], 255, MycRom.fWidth * MycRom.fHeight);
                        }
                    }
                }
            }
            else if ((GetKeyState(VK_MENU) & 0x8000) && (!Paste_Mode) && (!Color_Pipette))
            {
                if ((isDReleased) && (GetKeyState('D') & 0x8000) && (Edit_Mode == 1))
                {
                    SaveAction(true, SA_DYNAMASK);
                    for (UINT ti = 0; ti < nSelFrames; ti++)
                    {
                        for (UINT tj = 0; tj < MycRom.fWidth * MycRom.fHeight; tj++)
                        {
                            if (MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight + tj] < 255)
                                MycRom.cFrames[SelFrames[ti] * MycRom.fHeight * MycRom.fWidth + tj] = MycRP.oFrames[SelFrames[ti] * MycRom.fHeight * MycRom.fWidth + tj];
                        }
                        memset(&MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], 255, MycRom.fWidth * MycRom.fHeight);
                    }
                }
                if ((isDReleased) && (GetKeyState('A') & 0x8000) && (Edit_Mode == 1))
                {
                    SaveAction(true, SA_DYNAMASK);
                    for (UINT ti = 0; ti < nSelFrames; ti++)
                    {
                        memset(&MycRom.DynaMasks[SelFrames[ti] * MycRom.fWidth * MycRom.fHeight], acDynaSet, MycRom.fWidth * MycRom.fHeight);
                    }
                }
            }
            else
            {
                if ((isEnterReleased) && (GetKeyState(VK_RETURN) & 0x8000) && (Paste_Mode == true))
                {
                    mouse_button_callback(glfwframe, GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, 0);
                }
            }
        }
    }
    else if (GetForegroundWindow() == hSprites)
    {
        if (MycRom.name[0])
        {
            if ((GetKeyState(VK_CONTROL) & 0x8000))
            {
                if ((isZReleased) && (GetKeyState('Z') & 0x8000))
                {
                    isZReleased = false;
                    RecoverAction(true);
                }
                if ((isYReleased) && (GetKeyState('Y') & 0x8000))
                {
                    isYReleased = false;
                    RecoverAction(false);
                }
                if ((isSReleased) && (GetKeyState('S') & 0x8000))
                {
                    isSReleased = false;
                    Save_cRP(false);
                    Save_cRom(false);
                }
                if ((isVReleased) && (GetKeyState('V') & 0x8000))
                {
                    isVReleased = false;
                    if (IsWindowEnabled(GetDlgItem(hwTB2, IDC_PASTE)) == TRUE) SendMessage(hwTB2, WM_COMMAND, IDC_PASTE, 0);
                }
            }
        }
    }
}

void mouse_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (MycRom.name[0] == 0) return;
    short step = (short)yoffset;
    if (!(GetKeyState(VK_SHIFT) & 0x8000)) PreFrameInStrip -= step;
    else PreFrameInStrip -= step * (int)NFrameToDraw;
    if (PreFrameInStrip < 0) PreFrameInStrip = 0;
    if (PreFrameInStrip >= (int)MycRom.nFrames) PreFrameInStrip = (int)MycRom.nFrames - 1;
    UpdateFSneeded = true;
}

void mouse_scroll_callback2(GLFWwindow* window, double xoffset, double yoffset)
{
    if (MycRom.name[0] == 0) return;
    short step = (short)yoffset;
    if (!(GetKeyState(VK_SHIFT) & 0x8000)) PreSpriteInStrip -= step;
    else PreSpriteInStrip -= step * (int)NSpriteToDraw;
    if (PreSpriteInStrip < 0) PreSpriteInStrip = 0;
    if (PreSpriteInStrip >= (int)MycRom.nSprites) PreSpriteInStrip = (int)MycRom.nSprites - 1;
    UpdateSSneeded = true;
}

void mouse_move_callback2(GLFWwindow* window, double xpos, double ypos)
{
    if (MycRom.name[0] == 0) return; // we have no project loaded, ignore
    if (window == glfwsprites)
    {
        if ((Mouse_Mode == 4) || (Mouse_Mode == 3))
        {
            MouseFinPosx = (int)(xpos - MAX_SPRITE_SIZE * sprite_zoom - SPRITE_INTERVAL) / sprite_zoom;
            MouseFinPosy = (int)ypos / sprite_zoom;
            if (MouseFinPosx < 0) MouseFinPosx = 0;
            if (MouseFinPosx >= MAX_SPRITE_SIZE) MouseFinPosx = MAX_SPRITE_SIZE - 1;
            if (MouseFinPosy < 0) MouseFinPosy = 0;
            if (MouseFinPosy >= (int)MAX_SPRITE_SIZE) MouseFinPosy = MAX_SPRITE_SIZE - 1;
            if (Mouse_Mode == 3)
            {
                UINT8 tcol = MycRP.Sprite_Edit_Colors[acSprite * 16 + noSprSel];
                MycRom.SpriteDescriptions[(acSprite * MAX_SPRITE_SIZE + MouseFinPosy) * MAX_SPRITE_SIZE + MouseFinPosx] &= 0xff00;
                MycRom.SpriteDescriptions[(acSprite * MAX_SPRITE_SIZE + MouseFinPosy) * MAX_SPRITE_SIZE + MouseFinPosx] |= (UINT16)tcol;
            }
            return;
        }
        else if (Mouse_Mode == 2)
        {
            MouseFinPosx = (int)xpos / sprite_zoom;
            MouseFinPosy = (int)ypos / sprite_zoom;
            if (MouseFinPosx < 0) MouseFinPosx = 0;
            if (MouseFinPosx >= MAX_SPRITE_SIZE) MouseFinPosx = MAX_SPRITE_SIZE - 1;
            if (MouseFinPosy < 0) MouseFinPosy = 0;
            if (MouseFinPosy >= (int)MAX_SPRITE_SIZE) MouseFinPosy = MAX_SPRITE_SIZE - 1;
            return;
        }
    }
}

void mouse_move_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (MycRom.name[0] == 0) return; // we have no project loaded, ignore
    if (Paste_Mode)
    {
        POINT tpt;
        GetCursorPos(&tpt);
        paste_offsetx = -(paste_centerx - tpt.x) / frame_zoom;
        paste_offsety = -(paste_centery - tpt.y) / frame_zoom;
    }
    else if ((window == glfwframe) && (!Color_Pipette))
    {
        MouseFinPosx = (int)xpos / frame_zoom;
        MouseFinPosy = (int)ypos / frame_zoom;
        if (MouseFinPosx < 0) MouseFinPosx = 0;
        if (MouseFinPosx >= (int)MycRom.fWidth) MouseFinPosx = MycRom.fWidth - 1;
        if (MouseFinPosy < 0) MouseFinPosy = 0;
        if (MouseFinPosy >= (int)MycRom.fHeight) MouseFinPosy = MycRom.fHeight - 1;
        switch (Mouse_Mode)
        {
            case 1:
            {
                if (!isDel_Mode) MycRom.CompMasks[(MycRom.CompMaskID[acFrame] * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx] = 1;
                else MycRom.CompMasks[(MycRom.CompMaskID[acFrame] * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx] = 0;
                break;
            }
            case 3:
            {
                if (!Copy_Mode)
                {
                    UINT8 tcol;
                    if (MycRP.DrawColMode == 1) tcol = acEditColors[MycRP.oFrames[(acFrame * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx]];
                    else tcol = acEditColors[noColSel];
                    for (UINT32 ti = 0; ti < nSelFrames; ti++)
                    {
                        if (isDel_Mode) MycRom.cFrames[(SelFrames[ti] * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx] = MycRP.oFrames[(acFrame * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx];
                        else MycRom.cFrames[(SelFrames[ti] * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx] = tcol;
                    }
                }
                else
                {
                    if ((GetKeyState(VK_SHIFT) & 0x8000) > 0) Copy_Mask[MouseFinPosx + MouseFinPosy * MycRom.fWidth] = 0;
                    else Copy_Mask[MouseFinPosx + MouseFinPosy * MycRom.fWidth] = 1;
                }
                break;
            }
            case 5:
            {
                for (UINT32 ti = 0; ti < nSelFrames; ti++)
                {
                    if (isDel_Mode)
                    {
                        MycRom.DynaMasks[(SelFrames[ti] * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx] = 255;
                        MycRom.cFrames[(SelFrames[ti] * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx] =
                            MycRP.oFrames[(SelFrames[ti] * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx];
                    }
                    else MycRom.DynaMasks[(SelFrames[ti] * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx] = acDynaSet;
                }
                break;
            }
            case 7:
            {
                if (isDel_Mode) Copy_Mask[MouseFinPosy * MycRom.fWidth + MouseFinPosx] = 0;
                else
                {
                    if (!(GetKeyState('W') & 0x8000) || (MycRom.cFrames[(acFrame * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx] != 0))
                        Copy_Mask[MouseFinPosy * MycRom.fWidth + MouseFinPosx] = 1;
                }
                break;
            }
        }
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int xipos = (int)xpos, yipos = (int)ypos;

    if (MycRom.name[0] == 0) return; // we have no project loaded, ignore
    if ((action == GLFW_PRESS) && (Mouse_Mode != 0)) return; // we are already working on something else
    if (window == glfwframe) // we are on the frame
    {
        int xgrid, ygrid;
        xgrid = xipos / frame_zoom;
        ygrid = yipos / frame_zoom;
        if (action == GLFW_PRESS) // at button press time
        {
            if (Edit_Mode == 0) // comparison mode
            {
                if ((button == GLFW_MOUSE_BUTTON_LEFT) && (!(mods & (GLFW_MOD_ALT | GLFW_MOD_CONTROL))) && (MycRom.CompMaskID[acFrame] < 255))
                {
                    // we start editing the comparison mask
                    isDel_Mode = false;
                    if (mods && GLFW_MOD_SHIFT) isDel_Mode = true;
                    if (MycRP.Mask_Sel_Mode == 0)
                    {
                        // point mode
                        SaveAction(true, SA_COMPMASK);
                        Mouse_Mode = 1;
                        if (!isDel_Mode) MycRom.CompMasks[(MycRom.CompMaskID[acFrame] * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid] = 1;
                        else MycRom.CompMasks[(MycRom.CompMaskID[acFrame] * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid] = 0;
                    }
                    else
                    {
                        // rectangle or magic wand mode
                        Mouse_Mode = 2;
                        MouseIniPosx = xgrid;
                        MouseIniPosy = ygrid;
                        MouseFinPosx = xgrid;
                        MouseFinPosy = ygrid;
                    }
                    return;
                }
            }
            else // colorizing mode
            {
                // if pipette mode, we update the selected color with the pixel clicked
                if (Color_Pipette)
                {
                    if (button == GLFW_MOUSE_BUTTON_LEFT)
                    {
                        SaveAction(true, SA_EDITCOLOR);
                        acEditColors[noColSel] = MycRom.cFrames[(acFrame * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid];
                        InvalidateRect(GetDlgItem(hwTB, IDC_COL1 + noColSel), NULL, FALSE);
                        Color_Pipette = false;
                        glfwSetCursor(glfwframe, glfwarrowcur);
                    }
                    return;
                }
                else if (Paste_Mode)
                {
                    SaveAction(true, SA_DYNACOLOR);
                    SaveAction(true, SA_PALETTE);
                    SaveAction(true, SA_DRAW);
                    for (UINT32 tk = 0; tk < nSelFrames; tk++)
                    {
                        CopyColAndDynaCol(Copy_From_Frame, SelFrames[tk]);
                        for (UINT tj = 0; tj < Paste_Height; tj++)
                        {
                            for (UINT ti = 0; ti < Paste_Width; ti++)
                            {
                                int i = ti, j = tj;
                                if (Paste_Mirror & 1) i = Paste_Width - 1 - ti;
                                if (Paste_Mirror & 2) j = Paste_Height - 1 - tj;
                                if (Paste_Mask[j * Paste_Width + i] == 0) continue;
                                if (((ti + paste_offsetx) < 0) || ((ti + paste_offsetx) >= MycRom.fWidth) || ((tj + paste_offsety) < 0) || ((tj + paste_offsety) >= MycRom.fHeight)) continue;
                                MycRom.DynaMasks[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight + (tj + paste_offsety) * MycRom.fWidth + (ti + paste_offsetx)] = Paste_Dyna[i + j * Paste_Width];
                                MycRom.cFrames[SelFrames[tk] * MycRom.fWidth * MycRom.fHeight + (tj + paste_offsety) * MycRom.fWidth + (ti + paste_offsetx)] = Paste_Col[i + j * Paste_Width];
                            }
                        }
                    }
                    Paste_Mode = false;
                    glfwSetCursor(glfwframe, glfwarrowcur);
                    UpdateFSneeded = true;
                }
                else if ((button == GLFW_MOUSE_BUTTON_LEFT) && (!(mods & (GLFW_MOD_ALT | GLFW_MOD_CONTROL))))
                {
                    // else we start colorizing the frame
                    // if we are in multi frame edit mode, we check that the editing colors are the same
                    if (nSelFrames > 1)
                    {
                        bool isdifcol = false;
                        for (UINT ti = 0; ti < nSelFrames; ti++)
                        {
                            if (SelFrames[ti] == acFrame) continue;
                            for (UINT tj = 0; tj < 4; tj++)
                            {
                                if ((MycRom.cPal[SelFrames[ti] * 3 * MycRom.ncColors + 3 * acEditColors[tj]] != MycRom.cPal[acFrame * 3 * MycRom.ncColors + 3 * acEditColors[tj]]) ||
                                    (MycRom.cPal[SelFrames[ti] * 3 * MycRom.ncColors + 3 * acEditColors[tj] + 1] != MycRom.cPal[acFrame * 3 * MycRom.ncColors + 3 * acEditColors[tj] + 1]) ||
                                    (MycRom.cPal[SelFrames[ti] * 3 * MycRom.ncColors + 3 * acEditColors[tj] + 2] != MycRom.cPal[acFrame * 3 * MycRom.ncColors + 3 * acEditColors[tj] + 2]))
                                    isdifcol = true;
                                if (isdifcol) break;
                            }
                            if (isdifcol) break;
                        }
                        if (isdifcol)
                        {
                            if (MessageBoxA(hWnd, "You are about to draw on different frames with colours that are different, do you want to copy the 4 edit colors from the current frame palette to all the selected frame palettes?", "Caution", MB_YESNO) == IDYES)
                            {
                                for (UINT ti = 0; ti < nSelFrames; ti++)
                                {
                                    if (SelFrames[ti] == acFrame) continue;
                                    for (UINT tj = 0; tj < 4; tj++)
                                    {
                                        MycRom.cPal[SelFrames[ti] * 3 * MycRom.ncColors + 3 * acEditColors[tj]] = MycRom.cPal[acFrame * 3 * MycRom.ncColors + 3 * acEditColors[tj]];
                                        MycRom.cPal[SelFrames[ti] * 3 * MycRom.ncColors + 3 * acEditColors[tj] + 1] = MycRom.cPal[acFrame * 3 * MycRom.ncColors + 3 * acEditColors[tj] + 1];
                                        MycRom.cPal[SelFrames[ti] * 3 * MycRom.ncColors + 3 * acEditColors[tj] + 2] = MycRom.cPal[acFrame * 3 * MycRom.ncColors + 3 * acEditColors[tj] + 2];
                                    }
                                }
                            }
                        }
                    }
                    isDel_Mode = false;
                    if (mods & GLFW_MOD_SHIFT) isDel_Mode = true;
                    if (MycRP.Draw_Mode == 0)
                    {
                        // point mode
                        SaveAction(true, SA_DRAW);
                        Mouse_Mode = 3;
                        UINT8 tcol;
                        if (MycRP.DrawColMode == 1) tcol = acEditColors[MycRP.oFrames[(acFrame * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid]];
                        else tcol = acEditColors[noColSel];
                        for (UINT32 ti = 0; ti < nSelFrames; ti++)
                        {
                            if (isDel_Mode) MycRom.cFrames[(SelFrames[ti] * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid] = MycRP.oFrames[(acFrame * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid];
                            else MycRom.cFrames[(SelFrames[ti] * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid] = tcol;
                        }
                    }
                    else
                    {
                        // line, rectangle, circle or fill mode
                        Mouse_Mode = 4;
                        MouseIniPosx = xgrid;
                        MouseIniPosy = ygrid;
                        MouseFinPosx = xgrid;
                        MouseFinPosy = ygrid;
                    }
                    return;
                }
                else if (button == GLFW_MOUSE_BUTTON_LEFT)
                {
                    if (!(mods & GLFW_MOD_ALT))
                    {// we are editing the mask for copy mode
                        isDel_Mode = false;
                        if (mods & GLFW_MOD_SHIFT) isDel_Mode = true;
                        if (MycRP.Draw_Mode == 0)
                        {
                            // point mode
                            SaveAction(true, SA_COPYMASK);
                            Mouse_Mode = 7;
                            int ti = ygrid * MycRom.fWidth + xgrid;
                            if (!isDel_Mode) Copy_Mask[ti] = 1;
                            else Copy_Mask[ti] = 0;
                            Copy_Col[ti] = MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti];
                            Copy_Colo[ti] = MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti];
                            Copy_Dyna[ti] = MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight + ti];
                        }
                        else
                        {
                            // line, rectangle, circle or fill mode
                            Mouse_Mode = 8;
                            MouseIniPosx = xgrid;
                            MouseIniPosy = ygrid;
                            MouseFinPosx = xgrid;
                            MouseFinPosy = ygrid;
                        }
                        return;
                    }
                    else
                    { 
                        if (MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight + ygrid * MycRom.fWidth + xgrid] != 255)
                        {
                            MessageBoxA(hWnd, "The pixel you clicked is dynamically colored, this function works only for static content.", "Improper use", MB_OK);
                            return;
                        }
                        if (mods & GLFW_MOD_CONTROL)
                        {
                            // we select all the pixels with the same color as the one clicked!
                            SaveAction(true, SA_COPYMASK);
                            if (mods & GLFW_MOD_SHIFT) isDel_Mode = true;
                            Mouse_Mode = 7;
                            UINT8 col_to_find = MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ygrid * MycRom.fWidth + xgrid];
                            for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
                            {
                                if (MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti] == col_to_find)
                                {
                                    if (MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight + ti] != 255) continue;
                                    if (!isDel_Mode) Copy_Mask[ti] = 1;
                                    else Copy_Mask[ti] = 0;
                                    Copy_Col[ti] = MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti];
                                    Copy_Colo[ti] = MycRP.oFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ti];
                                    Copy_Dyna[ti] = MycRom.DynaMasks[acFrame * MycRom.fWidth * MycRom.fHeight + ti];
                                }
                            }
                        }
                        else
                        {
                            // we colors all the pixels with the same color as the one clicked
                            SaveAction(true, SA_DRAW);
                            Mouse_Mode = 3;
                            UINT8 col_to_find = MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight + ygrid * MycRom.fWidth + xgrid];
                            for (UINT tj = 0; tj < nSelFrames; tj++)
                            {
                                for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++)
                                {
                                    if (MycRom.cFrames[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti] == col_to_find)
                                    {
                                        if (MycRP.DrawColMode == 1) MycRom.cFrames[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti] = acEditColors[MycRP.oFrames[SelFrames[tj] * MycRom.fHeight * MycRom.fWidth + ti]];
                                        else MycRom.cFrames[SelFrames[tj] * MycRom.fWidth * MycRom.fHeight + ti] = acEditColors[noColSel];
                                    }
                                }
                            }
                        }
                        return;
                    }

                }
                else if ((button == GLFW_MOUSE_BUTTON_RIGHT) && (!(mods & (GLFW_MOD_ALT | GLFW_MOD_CONTROL))))
                {
                    // we start editing the dynamic mask
                    isDel_Mode = false;
                    if (mods & GLFW_MOD_SHIFT) isDel_Mode = true;
                    if (MycRP.Draw_Mode == 0)
                    {
                        // point mode
                        SaveAction(true, SA_DYNAMASK);
                        Mouse_Mode = 5;
                        for (UINT32 ti = 0; ti < nSelFrames; ti++)
                        {
                            if (!isDel_Mode) MycRom.DynaMasks[(SelFrames[ti] * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid] = acDynaSet;
                            else
                            {
                                    if (MycRom.DynaMasks[(SelFrames[ti] * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid] < 255)
                                        MycRom.cFrames[(SelFrames[ti] * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid] =
                                        MycRP.oFrames[(SelFrames[ti] * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid];
                                MycRom.DynaMasks[(SelFrames[ti] * MycRom.fHeight + ygrid) * MycRom.fWidth + xgrid] = 255;
                            }
                        }
                    }
                    else
                    {
                        // rectangle or magic wand mode
                        Mouse_Mode = 6;
                        MouseIniPosx = xgrid;
                        MouseIniPosy = ygrid;
                        MouseFinPosx = xgrid;
                        MouseFinPosy = ygrid;
                    }
                    return;
                }
            }
        }
        else if ((action == GLFW_RELEASE) && (!Color_Pipette) && (!Paste_Mode))
        {
            if (Edit_Mode == 0)
            {
                // we release the button for comparison mask
                if ((button == GLFW_MOUSE_BUTTON_LEFT) && (Mouse_Mode == 2))
                {
                    SaveAction(true, SA_COMPMASK);
                    Add_Surface_To_Mask(Draw_Extra_Surface, isDel_Mode);
                }
            }
            else
            {
                // we release the button for colorization 
                if ((button == GLFW_MOUSE_BUTTON_LEFT) && (Mouse_Mode == 4))
                {
                    SaveAction(true, SA_DRAW);
                    if (MycRP.DrawColMode == 2)
                    {
                        if (MycRP.Draw_Mode==1) ConvertCopyToGradient(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy);
                        else ConvertCopyToRadialGradient(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy);
                    }
                    else ConvertSurfaceToFrame(Draw_Extra_Surface, isDel_Mode);
                    UpdateFSneeded = true;
                }
                else if ((button == GLFW_MOUSE_BUTTON_LEFT) && (Mouse_Mode == 3)) UpdateFSneeded = true;
                else if ((button == GLFW_MOUSE_BUTTON_LEFT) && (Mouse_Mode == 8))
                {
                    SaveAction(true, SA_COPYMASK);
                    Add_Surface_To_Copy(Draw_Extra_Surface, isDel_Mode);
                }
                // we release the button for dynamic mask
                else if ((button == GLFW_MOUSE_BUTTON_RIGHT) && (Mouse_Mode == 6))
                {
                    SaveAction(true, SA_DYNAMASK);
                    Add_Surface_To_Dyna(Draw_Extra_Surface, isDel_Mode);
                }
            }
            Mouse_Mode = 0;
            return;
        }
    }
    else if ((window == glfwframestrip) && (MycRom.name[0] != 0))
    {
        // Click on the frame strip
        if ((button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_RELEASE)) MouseFrSliderLPressed = false;
        if ((button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_PRESS))
        {
            // are we on the slider
            if ((yipos >= FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 - 5) && (yipos <= FRAME_STRIP_HEIGHT - FRAME_STRIP_H_MARGIN / 2 + 6))
            {
                if ((xipos >= FRAME_STRIP_SLIDER_MARGIN) && (xipos <= (int)ScrW - FRAME_STRIP_SLIDER_MARGIN))
                {
                    MouseFrSliderLPressed = true;
                    MouseFrSliderlx = xipos - FRAME_STRIP_SLIDER_MARGIN;
                }
                return;
            }
            // are we on a frame of the strip to select
            UINT wid = 256;
            if (MycRom.fWidth == 192) wid = 192;
            if ((yipos >= FRAME_STRIP_H_MARGIN) && (yipos < FRAME_STRIP_H_MARGIN + 128))
            {
                if ((xipos >= (int)FS_LMargin) && (xipos <= (int)ScrW - (int)FS_LMargin))
                {
                    if ((xipos - FS_LMargin) % (wid + FRAME_STRIP_W_MARGIN) < wid) // check that we are not between the frames
                    {
                        SaveAction(true, SA_SELECTION);
                        prevFrame = acFrame;
                        acFrame = (UINT)-1;
                        bool frameshift = false;
                        UINT tpFrame = PreFrameInStrip + (xipos - FS_LMargin) / (wid + FRAME_STRIP_W_MARGIN);
                        if (tpFrame >= MycRom.nFrames) tpFrame = MycRom.nFrames - 1;
                        if (tpFrame < 0) tpFrame = 0;
                        if (mods & GLFW_MOD_SHIFT)
                        {
                            // range of frame to de/select
                            if (mods & GLFW_MOD_CONTROL)
                            {
                                // add this range to the current selection
                                for (UINT ti = min(prevFrame, tpFrame); ti <= max(prevFrame, tpFrame); ti++) Add_Selection_Frame(ti);
                            }
                            else if (mods & GLFW_MOD_ALT)
                            {
                                // remove this range from the current selection
                                for (UINT ti = min(prevFrame, tpFrame); ti <= max(prevFrame, tpFrame); ti++) Del_Selection_Frame(ti);
                            }
                            else
                            {
                                // just select this range
                                nSelFrames = 0;
                                for (UINT ti = min(prevFrame, tpFrame); ti <= max(prevFrame, tpFrame); ti++) Add_Selection_Frame(ti);
                            }
                        }
                        else if (mods & GLFW_MOD_CONTROL)
                        {
                            // just add or remove one frame to the current selection
                            if ((isFrameSelected(tpFrame) != -1) && (nSelFrames > 1)) Del_Selection_Frame(tpFrame); else Add_Selection_Frame(tpFrame);
                        }
                        else
                        {
                            // selecting just this frame
                            // if we change the frame in the selection range, we keep the selection range as it is
                            int iSF = isFrameSelected(tpFrame);
                            if ((iSF == -1) || (tpFrame == prevFrame))
                            {
                                SelFrames[0] = tpFrame;
                                nSelFrames = 1;
                            }
                            else frameshift = true;
                        }
                        if (isFrameSelected(tpFrame) == -1)
                        {
                            acFrame = SelFrames[0];
                        }
                        else acFrame = tpFrame;
                        InitColorRotation();

                        if (Edit_Mode == 1)
                        {
                            for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
                            for (UINT ti = IDC_DYNACOL1; ti <= IDC_DYNACOL16; ti++) InvalidateRect(GetDlgItem(hwTB, ti), NULL, TRUE);
                        }
                        if (!frameshift)
                        {
                            for (UINT ti = 0; ti < MycRom.fWidth * MycRom.fHeight; ti++) Copy_Mask[ti] = 0;
                        }
                        Paste_Mode = false;
                        UpdateFSneeded = true;
                        Deactivate_Draw_All_Sel();
                        UpdateNewacFrame();
                    }
                }
            }
        }
        return;
    }
}

void mouse_button_callback2(GLFWwindow* window, int button, int action, int mods)
{
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    int xipos = (int)xpos, yipos = (int)ypos;

    if (MycRom.name[0] == 0) return; // we have no project loaded, ignore
    if (window == glfwsprites) // we are on the sprite
    {
        int xgrid, ygrid;
        ygrid = yipos / sprite_zoom;
        if (xipos >= MAX_SPRITE_SIZE * sprite_zoom - SPRITE_INTERVAL)
        {
            xgrid = (xipos - MAX_SPRITE_SIZE * sprite_zoom - SPRITE_INTERVAL) / sprite_zoom;
            if ((action == GLFW_PRESS) && (button == GLFW_MOUSE_BUTTON_LEFT))
            {
                if (Sprite_Mode == 0)
                {
                    // point mode
                    SaveAction(true, SA_SPRITE);
                    Mouse_Mode = 3;
                    UINT8 tcol = MycRP.Sprite_Edit_Colors[acSprite * 16 + noSprSel];
                    MycRom.SpriteDescriptions[(acSprite * MAX_SPRITE_SIZE + ygrid) * MAX_SPRITE_SIZE + xgrid] &= 0xff00;
                    MycRom.SpriteDescriptions[(acSprite * MAX_SPRITE_SIZE + ygrid) * MAX_SPRITE_SIZE + xgrid] |= (UINT16)tcol;
                }
                else
                {
                    // line, rectangle, circle or fill mode
                    Mouse_Mode = 4;
                    MouseIniPosx = xgrid;
                    MouseIniPosy = ygrid;
                    MouseFinPosx = xgrid;
                    MouseFinPosy = ygrid;
                }
                return;
            }
            else if (action == GLFW_RELEASE)
            {
                // we release the button for colorization 
                if ((button == GLFW_MOUSE_BUTTON_LEFT) && (Mouse_Mode == 4))
                {
                    SaveAction(true, SA_SPRITE);
                    ConvertSurfaceToSprite(Draw_Extra_Surface2);
                    UpdateSSneeded = true;
                }
                else if ((button == GLFW_MOUSE_BUTTON_LEFT) && (Mouse_Mode == 3)) UpdateSSneeded = true;
                Mouse_Mode = 0;
                return;
            }
        }
        else if (xipos < MAX_SPRITE_SIZE * sprite_zoom )
        {
            xgrid = xipos / sprite_zoom;
            if ((action == GLFW_PRESS) && (button == GLFW_MOUSE_BUTTON_LEFT))
            {
                Mouse_Mode = 2;
                MouseIniPosx = xgrid;
                MouseIniPosy = ygrid;
                MouseFinPosx = xgrid;
                MouseFinPosy = ygrid;
                return;
            }
            else if (action == GLFW_RELEASE)
            {
                // we release the button for colorization 
                if ((button == GLFW_MOUSE_BUTTON_LEFT) && (Mouse_Mode == 2))
                {
                    UINT nsuiv = 0;
                    Mouse_Mode = 0;
                    for (UINT ti = 0; ti < MAX_SPRITE_SIZE * MAX_SPRITE_SIZE-3; ti++)
                    {
                        if ((Draw_Extra_Surface2[ti] > 0) && ((MycRom.SpriteDescriptions[acSprite * MAX_SPRITE_SIZE * MAX_SPRITE_SIZE + ti] & 0xff00) != 0xff00)) nsuiv++; else nsuiv = 0;
                        if (nsuiv == 4) break;
                    }
                    if (nsuiv < 4)
                    {
                        MessageBoxA(hSprites, "This selection doesn't have a sequence of 4 consecutive pixels active in the sprite, it can't be used for identification", "Invalid selection", MB_OK);
                        return;
                    }
                    SaveAction(true, SA_SPRITE);
                    ConvertSurfaceToDetection(Draw_Extra_Surface2);
                }
                return;
            }
        }
    }
    else if ((window == glfwspritestrip) && (MycRom.name[0] != 0))
    {
        // Click on the frame strip
        if ((button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_RELEASE)) MouseFrSliderLPressed = false;
        if ((button == GLFW_MOUSE_BUTTON_LEFT) && (action == GLFW_PRESS))
        {
           /* // are we on the slider
            if ((yipos >= FRAME_STRIP_HEIGHT2 - FRAME_STRIP_H_MARGIN / 2 - 5) && (yipos <= FRAME_STRIP_HEIGHT2 - FRAME_STRIP_H_MARGIN / 2 + 6))
            {
                if ((xipos >= FRAME_STRIP_SLIDER_MARGIN) && (xipos <= (int)ScrW - FRAME_STRIP_SLIDER_MARGIN))
                {
                    MouseFrSliderLPressed = true;
                    MouseFrSliderlx = xipos - FRAME_STRIP_SLIDER_MARGIN;
                }
                return;
            }*/
            // are we on a frame of the strip to select
            UINT wid = MAX_SPRITE_SIZE*2;
            if ((yipos >= FRAME_STRIP_H_MARGIN) && (yipos < FRAME_STRIP_H_MARGIN + MAX_SPRITE_SIZE))
            {
                if ((xipos >= (int)SS_LMargin) && (xipos <= (int)ScrW2 - (int)SS_LMargin))
                {
                    if ((xipos - SS_LMargin) % (wid + FRAME_STRIP_W_MARGIN) < wid) // check that we are not between the frames
                    {
                        SaveAction(true, SA_ACSPRITE);
                        UINT tnspr = PreSpriteInStrip + (xipos - SS_LMargin) / (wid + FRAME_STRIP_W_MARGIN);
                        if ((tnspr < 0) || (tnspr >= MycRom.nSprites)) return;
                        acSprite = tnspr;
                        // selecting just this frame
                        // if we change the frame in the selection range, we keep the selection range as it is
                        for (UINT ti = IDC_COL1; ti <= IDC_COL16; ti++) InvalidateRect(GetDlgItem(hwTB2, ti), NULL, TRUE);
                        Paste_Mode = false;
                        UpdateSSneeded = true;
                        SendMessage(GetDlgItem(hwTB2, IDC_SPRITELIST), CB_SETCURSEL, (WPARAM)acSprite, 0);
                        SetDlgItemTextA(hwTB2, IDC_SPRITENAME, &MycRP.Sprite_Names[acSprite * SIZE_SECTION_NAMES]);
                    }
                }
            }
        }
        return;
    }
}

bool isPressed(int nVirtKey, DWORD* timePress)
{
    // Check if a key is pressed, manage repetition (first delay longer than the next ones)
    if (GetKeyState(nVirtKey) & 0x8000)
    {
        DWORD acTime = timeGetTime();
        if (acTime > (*timePress))
        {
            if ((*timePress) == 0) (*timePress) = FIRST_KEY_TIMER_INT + acTime; else(*timePress) = NEXT_KEY_TIMER_INT + acTime;
            return true;
        }
    }
    else (*timePress) = 0;
    return false;
}

#pragma endregion Window_Procs

#pragma region Window_Creations

bool SetIcon(HWND ButHWND, UINT ButIco)
{
    HICON hicon = LoadIcon(hInst, MAKEINTRESOURCE(ButIco));
    if (!hicon) AffLastError((char*)"SetIcon");
    SendMessageW(ButHWND, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hicon);
    DestroyIcon(hicon);
    return true;
}

bool SetImage(HWND ButHWND, UINT ButImg)
{
    HBITMAP hbitmap = (HBITMAP)LoadBitmap(hInst, MAKEINTRESOURCE(ButImg));
    if (!hbitmap) AffLastError((char*)"SetImage");
    SendMessageW(ButHWND, BM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hbitmap);
    DeleteObject(hbitmap);
    return true;
}

bool MaskUsed(UINT32 nomask)
{
    if (MycRom.name[0] == 0) return false;
    for (UINT32 ti = 0; ti < MycRom.nFrames; ti++)
    {
        if (MycRom.CompMaskID[ti] == nomask) return true;
    }
    return false;
}

void UpdateSectionList(void)
{
    HWND hlst = GetDlgItem(hwTB, IDC_SECTIONLIST);
    SendMessage(hlst, CB_RESETCONTENT, 0, 0);
    SendMessageA(hlst, CB_ADDSTRING, 0, (LPARAM)"- None -");
    for (UINT32 ti = 0; ti < MycRP.nSections; ti++)
    {
        char tbuf[256];
        sprintf_s(tbuf, 256, "%i - %s", ti + 1, &MycRP.Section_Names[ti * SIZE_SECTION_NAMES]);
        SendMessageA(hlst, CB_ADDSTRING, 0, (LPARAM)tbuf);
    }
    int ti = Which_Section(acFrame);
    SendMessage(hlst, CB_SETCURSEL, ti + 1, 0);
}

void UpdateSpriteList(void)
{
    HWND hlst = GetDlgItem(hwTB2, IDC_SPRITELIST);
    //acSprite = (UINT)SendMessage(hlst, LB_GETCURSEL, 0, 0);
    SendMessage(hlst, CB_RESETCONTENT, 0, 0);
    for (UINT32 ti = 0; ti < MycRom.nSprites; ti++)
    {
        char tbuf[256];
        sprintf_s(tbuf, 256, "%s", &MycRP.Sprite_Names[ti * SIZE_SECTION_NAMES]);
        SendMessageA(hlst, CB_ADDSTRING, 0, (LPARAM)tbuf);
    }
    SendMessage(hlst, CB_SETCURSEL, acSprite, 0);
}

void UpdateMaskList2(void)
{
    if (MycRom.name[0] == 0) return;
    HWND hlst = GetDlgItem(hwTB, IDC_MASKLIST2);
    HWND hlst2 = GetDlgItem(hwTB, IDC_FRUSEMASK);
    int acpos = (int)SendMessage(hlst, CB_GETCURSEL, 0, 0);
    SendMessage(hlst2, LB_RESETCONTENT, 0, 0);
    if (acpos >= 0)
    {
        for (UINT ti = 0; ti < MycRom.nFrames; ti++)
        {
            if (MycRom.CompMaskID[ti] == acpos)
            {
                char tbuf[12];
                _itoa_s(ti, tbuf, 12, 10);
                SendMessageA(hlst2, LB_ADDSTRING, 0, (LPARAM)tbuf);
            }
        }
    }
}

void UpdateSpriteList2(void)
{
    if (MycRom.name[0] == 0) return;
    HWND hlst = GetDlgItem(hwTB2, IDC_SPRITELIST2);
    HWND hlst2 = GetDlgItem(hwTB2, IDC_FRUSESPRITE);
    int acpos = (int)SendMessage(hlst, CB_GETCURSEL, 0, 0);
    SendMessage(hlst2, LB_RESETCONTENT, 0, 0);
    if (acpos >= 0)
    {
        for (UINT ti = 0; ti < MycRom.nFrames; ti++)
        {
            for (UINT tj = 0; tj < MAX_SPRITES_PER_FRAME; tj++)
            {
                if (MycRom.FrameSprites[ti * MAX_SPRITES_PER_FRAME + tj] == acpos)
                {
                    char tbuf[12];
                    _itoa_s(ti, tbuf, 12, 10);
                    SendMessageA(hlst2, LB_ADDSTRING, 0, (LPARAM)tbuf);
                }
            }
        }
    }
}

void UpdateSpriteList3(void)
{
    if (MycRom.name[0] == 0) return;
    HWND hlst = GetDlgItem(hwTB2, IDC_SPRITELIST2);
    SendMessage(hlst, CB_RESETCONTENT, 0, 0);
    for (UINT ti = 0; ti < MycRom.nSprites; ti++) SendMessageA(hlst, CB_ADDSTRING, 0, (LPARAM)&MycRP.Sprite_Names[ti * SIZE_SECTION_NAMES]);
    UpdateSpriteList2();
}

void UpdateMaskList(void)
{
    HWND hlst = GetDlgItem(hwTB, IDC_MASKLIST);
    HWND hlst2 = GetDlgItem(hwTB, IDC_MASKLIST2);
    SendMessage(hlst, CB_RESETCONTENT, 0, 0);
    SendMessageA(hlst, CB_ADDSTRING, 0, (LPARAM)"- None -");
    SendMessage(hlst2, CB_RESETCONTENT, 0, 0);
    wchar_t tbuf[256],tname[SIZE_MASK_NAME];
    size_t tout;
    for (UINT32 ti = 0; ti < MAX_MASKS; ti++)
    {
        if (MaskUsed(ti))
        {
            if (MycRP.Mask_Names[ti * SIZE_MASK_NAME] != 0)
                mbstowcs_s(&tout, tname, &MycRP.Mask_Names[ti * SIZE_MASK_NAME], SIZE_MASK_NAME - 1);
            else
                _itow_s(ti, tname, SIZE_MASK_NAME - 1, 10);
            swprintf_s(tbuf, 256, L"\u2611 %s", tname);
        }
        else
            swprintf_s(tbuf, 256, L"\u2610 %i", ti);
        SendMessage(hlst, CB_ADDSTRING, 0, (LPARAM)tbuf);
        SendMessage(hlst2, CB_ADDSTRING, 0, (LPARAM)tbuf);
    }
    if (MycRom.name[0])
    {
        UINT8 puc = MycRom.CompMaskID[acFrame];
        puc++;
        SendMessage(GetDlgItem(hwTB, IDC_MASKLIST), CB_SETCURSEL, (WPARAM)puc, 0);
        UpdateMaskList2();
    }
}

void UpdateTriggerID(void)
{
    if (MycRom.name[0] == 0) return;
    if (MycRom.TriggerID[acFrame] == 0xFFFFFFFF) SetDlgItemTextA(hwTB, IDC_TRIGID, "- None -");
    else
    {
        char tbuf[16];
        _itoa_s(MycRom.TriggerID[acFrame], tbuf, 16, 10);
        SetDlgItemTextA(hwTB, IDC_TRIGID,tbuf);
    }
}

void SetSpotButton(bool ison)
{
    if (ison) SetIcon(GetDlgItem(hwTB, IDC_IDENT), IDI_SPOTON);
    else SetIcon(GetDlgItem(hwTB, IDC_IDENT), IDI_SPOTOFF);
}

bool CreateToolbar(void)
{
    if (hwTB)
    {
/*       if (functoremove == 1)
        {
            RemoveWindowSubclass(hwTB, (SUBCLASSPROC)SubclassMaskListProc, 0);
            RemoveWindowSubclass(hwTB, (SUBCLASSPROC)SubclassSecListoProc, 1);
            RemoveWindowSubclass(hwTB, (SUBCLASSPROC)SubclassMask2ListProc, 4);
        }
        else if (functoremove == 2)
        {
            RemoveWindowSubclass(hwTB, (SUBCLASSPROC)SubclassSecListcProc, 2);
            RemoveWindowSubclass(hwTB, (SUBCLASSPROC)SubclassSpriteList2Proc, 3);
        }*/
        DestroyWindow(hwTB);
    }
    if (Edit_Mode == 0)
    {
        hwTB = CreateDialog(hInst, MAKEINTRESOURCE(IDD_ORGDLG), hWnd, Toolbar_Proc);
        if (!hwTB)
        {
            cprintf("Unable to create the comparison toolbar");
            return false;
        }
        ShowWindow(hwTB, TRUE);
        SetIcon(GetDlgItem(hwTB, IDC_COLMODE), IDI_COLMODE);
        //CreateToolTip(IDC_COLMODE, hwTB, (PTSTR)L"Go to colorization mode");
        SetIcon(GetDlgItem(hwTB, IDC_NEW), IDI_NEW);
        SetIcon(GetDlgItem(hwTB, IDC_ADDTXT), IDI_ADDTXT);
        SetIcon(GetDlgItem(hwTB, IDC_OPEN), IDI_OPEN);
        SetIcon(GetDlgItem(hwTB, IDC_SAVE), IDI_SAVE);
        SetIcon(GetDlgItem(hwTB, IDC_UNDO), IDI_UNDO);
        SetIcon(GetDlgItem(hwTB, IDC_REDO), IDI_REDO);
        SetIcon(GetDlgItem(hwTB, IDC_POINTMASK), IDI_DRAWPOINT);
        SetIcon(GetDlgItem(hwTB, IDC_RECTMASK), IDI_DRAWRECT);
        SetIcon(GetDlgItem(hwTB, IDC_ZONEMASK), IDI_MAGICWAND);
        SetIcon(GetDlgItem(hwTB, IDC_CHECKFRAMES), IDI_SEARCH);
        SetIcon(GetDlgItem(hwTB, IDC_ADDSECTION), IDI_ADDTAB);
        SetIcon(GetDlgItem(hwTB, IDC_DELSECTION), IDI_DELTAB);
        SetIcon(GetDlgItem(hwTB, IDC_INVERTSEL), IDI_INVERTSEL);
        SetIcon(GetDlgItem(hwTB, IDC_DELALLSAMEFR), IDI_DELALLSAME);
        SetIcon(GetDlgItem(hwTB, IDC_DELSELSAMEFR), IDI_DELSELSAME);
        SetIcon(GetDlgItem(hwTB, IDC_DELFRAME), IDI_DELFRAME);
        SetIcon(GetDlgItem(hwTB, IDC_INVERTSEL), IDI_INVERTSEL);
        //SetIcon(GetDlgItem(hwTB, IDC_SELSAMEFR), IDI_SELSAMEFR);
        SetIcon(GetDlgItem(hwTB, IDC_DELSAMEFR), IDI_DELSAMEFR);
        SetIcon(GetDlgItem(hwTB, IDC_MOVESECTION), IDI_MOVESECTION);
        SetIcon(GetDlgItem(hwTB, IDC_NIGHTDAY), IDI_NIGHTDAY);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY2), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY2), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY3), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY3), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY4), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY4), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY12), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY12), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY14), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY14), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY17), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY17), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY18), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY18), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        UpdateMaskList();
        UpdateSectionList();
        UpdateTriggerID();
        if (MycRom.name[0] != 0)
        {
            if (MycRom.ShapeCompMode[acFrame] == TRUE) SendMessage(GetDlgItem(hwTB, IDC_SHAPEMODE), BM_SETCHECK, BST_CHECKED, 0);
            else SendMessage(GetDlgItem(hwTB, IDC_SHAPEMODE), BM_SETCHECK, BST_UNCHECKED, 0);
            CheckSameFrames();
        }
        /*oldMaskListFunc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwTB, IDC_MASKLIST), GWLP_WNDPROC);
        SetWindowSubclass(GetDlgItem(hwTB, IDC_MASKLIST), (SUBCLASSPROC)SubclassMaskListProc, 0, 0);
        oldSectionListoFunc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwTB, IDC_SECTIONLIST), GWLP_WNDPROC);
        SetWindowSubclass(GetDlgItem(hwTB, IDC_SECTIONLIST), (SUBCLASSPROC)SubclassSecListoProc, 1, 0);
        oldMask2ListFunc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwTB, IDC_MASKLIST2), GWLP_WNDPROC);
        SetWindowSubclass(GetDlgItem(hwTB, IDC_MASKLIST2), (SUBCLASSPROC)SubclassMask2ListProc, 4, 0);
        functoremove = 1;*/
    }
    else
    {
        hwTB = CreateDialog(hInst, MAKEINTRESOURCE(IDD_COLDLG), hWnd, Toolbar_Proc);
        if (!hwTB)
        {
            cprintf("Unable to create the colorization toolbar");
            return false;
        }
        ShowWindow(hwTB, TRUE);
        SetIcon(GetDlgItem(hwTB, IDC_ORGMODE), IDI_ORGMODE);
        SetIcon(GetDlgItem(hwTB, IDC_NEW), IDI_NEW);
        SetIcon(GetDlgItem(hwTB, IDC_ADDTXT), IDI_ADDTXT);
        SetIcon(GetDlgItem(hwTB, IDC_OPEN), IDI_OPEN);
        SetIcon(GetDlgItem(hwTB, IDC_SAVE), IDI_SAVE);
        SetIcon(GetDlgItem(hwTB, IDC_UNDO), IDI_UNDO);
        SetIcon(GetDlgItem(hwTB, IDC_REDO), IDI_REDO);
        SetIcon(GetDlgItem(hwTB, IDC_COLSET), IDI_COLSET);
        SetIcon(GetDlgItem(hwTB, IDC_DRAWPOINT), IDI_DRAWPOINT);
        SetIcon(GetDlgItem(hwTB, IDC_DRAWLINE), IDI_DRAWLINE);
        SetIcon(GetDlgItem(hwTB, IDC_DRAWRECT), IDI_DRAWRECT);
        SetIcon(GetDlgItem(hwTB, IDC_DRAWCIRC), IDI_DRAWCERC);
        SetIcon(GetDlgItem(hwTB, IDC_FILL), IDI_MAGICWAND);
        SetIcon(GetDlgItem(hwTB, IDC_COPYCOLS), IDI_4COLSCOPY);
        SetIcon(GetDlgItem(hwTB, IDC_COPYCOLS2), IDI_64COLSCOPY);
        SetIcon(GetDlgItem(hwTB, IDC_DYNACOLSET), IDI_COLSET);
        SetIcon(GetDlgItem(hwTB, IDC_COLPICK), IDI_COLPICK);
        SetIcon(GetDlgItem(hwTB, IDC_COPY), IDI_COPY);
        SetIcon(GetDlgItem(hwTB, IDC_PASTE), IDI_PASTE);
        SetIcon(GetDlgItem(hwTB, IDC_ADDSECTION), IDI_ADDTAB);
        SetIcon(GetDlgItem(hwTB, IDC_DELSECTION), IDI_DELTAB);
        SetIcon(GetDlgItem(hwTB, IDC_COLTODYNA), IDI_COLTODYNA);
        SetIcon(GetDlgItem(hwTB, IDC_INVERTSEL2), IDI_INVERTSEL);
        //SetIcon(GetDlgItem(hwTB, IDC_INVERTSEL3), IDI_INVERTSEL);
        SetIcon(GetDlgItem(hwTB, IDC_MOVESECTION), IDI_MOVESECTION);
        SetIcon(GetDlgItem(hwTB, IDC_ADDSPRITE2), IDI_ADDSPR);
        SetIcon(GetDlgItem(hwTB, IDC_DELSPRITE2), IDI_DELSPR);
        SetIcon(GetDlgItem(hwTB, IDC_NIGHTDAY), IDI_NIGHTDAY);
        SendMessage(GetDlgItem(hwTB, IDC_CHANGECOLSET), UDM_SETRANGE, 0, MAKELPARAM(MAX_DYNA_SETS_PER_FRAME-1,0));
        SendMessage(GetDlgItem(hwTB, IDC_CHANGECOLSET), UDM_SETPOS, 0, (LPARAM)acDynaSet);
        char tbuf[10];
        _itoa_s(acDynaSet + 1, tbuf, 8, 10);
        SetDlgItemTextA(hwTB, IDC_NOSETCOL, tbuf);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY5), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY5), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY6), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY6), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY7), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY7), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY8), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY8), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY9), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY9), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY10), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY10), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY11), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY11), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY13), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY13), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY15), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY15), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        SetWindowLong(GetDlgItem(hwTB, IDC_STRY16), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
        SetWindowPos(GetDlgItem(hwTB, IDC_STRY16), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
        if (MycRom.name[0] != 0)
        {
            if (MycRP.Fill_Mode == TRUE) SendMessage(GetDlgItem(hwTB, IDC_FILLED), BM_SETCHECK, BST_CHECKED, 0); else SendMessage(GetDlgItem(hwTB, IDC_FILLED), BM_SETCHECK, BST_UNCHECKED, 0);
            UpdateFrameSpriteList();
        }
        SetSpotButton(Ident_Pushed);
        /*oldSectionListcFunc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwTB, IDC_SECTIONLIST), GWLP_WNDPROC);
        SetWindowSubclass(GetDlgItem(hwTB, IDC_SECTIONLIST), (SUBCLASSPROC)SubclassSecListcProc, 2, 0);
        oldSpriteList2Func = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwTB, IDC_SPRITELIST2), GWLP_WNDPROC);
        SetWindowSubclass(GetDlgItem(hwTB, IDC_SPRITELIST2), (SUBCLASSPROC)SubclassSpriteList2Proc, 3, 0);
        functoremove = 2;*/
    }
    if (UndoAvailable > 0) EnableWindow(GetDlgItem(hwTB, IDC_UNDO), TRUE); else EnableWindow(GetDlgItem(hwTB, IDC_UNDO), FALSE);
    if (RedoAvailable > 0) EnableWindow(GetDlgItem(hwTB, IDC_REDO), TRUE); else EnableWindow(GetDlgItem(hwTB, IDC_REDO), FALSE);

    // to avoid beeps on key down
    UpdateSectionList();
    SetFocus(hwTB);
    PostMessage(hwTB, WM_NEXTDLGCTL, (WPARAM)GetDlgItem(hwTB, IDC_NEW), TRUE);

    return true;
}

bool CreateToolbar2(void)
{
    if (hwTB2)
    {
        /*RemoveWindowSubclass(hwTB2, (SUBCLASSPROC)SubclassSpriteListProc, 5);
        RemoveWindowSubclass(hwTB2, (SUBCLASSPROC)SubclassSpriteDetListProc, 6);*/
        DestroyWindow(hwTB2);
    }
    hwTB2 = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SPRDLG), hSprites, Toolbar_Proc2);
    if (!hwTB2)
    {
        cprintf("Unable to create the comparison toolbar");
        return false;
    }
    ShowWindow(hwTB2, TRUE);
    SetIcon(GetDlgItem(hwTB2, IDC_SAVE), IDI_SAVE);
    SetIcon(GetDlgItem(hwTB2, IDC_PASTE), IDI_PASTE);
    SetIcon(GetDlgItem(hwTB2, IDC_ADDSPRITE), IDI_ADDSPR);
    SetIcon(GetDlgItem(hwTB2, IDC_DELSPRITE), IDI_DELSPR);
    SetIcon(GetDlgItem(hwTB2, IDC_DRAWPOINT), IDI_DRAWPOINT);
    SetIcon(GetDlgItem(hwTB2, IDC_DRAWLINE), IDI_DRAWLINE);
    SetIcon(GetDlgItem(hwTB2, IDC_DRAWRECT), IDI_DRAWRECT);
    SetIcon(GetDlgItem(hwTB2, IDC_DRAWCIRC), IDI_DRAWCERC);
    SetIcon(GetDlgItem(hwTB2, IDC_FILL), IDI_MAGICWAND);
    SetIcon(GetDlgItem(hwTB2, IDC_UNDO), IDI_UNDO);
    SetIcon(GetDlgItem(hwTB2, IDC_REDO), IDI_REDO);
    SetIcon(GetDlgItem(hwTB2, IDC_ADDTOFRAME), IDI_ADDSPRFR);
    SetIcon(GetDlgItem(hwTB2, IDC_DELDETSPR), IDI_DELDETSPR);
    SetIcon(GetDlgItem(hwTB2, IDC_TOFRAME), IDI_SPRTOFRAME);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY1), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY1), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY2), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY2), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY3), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY3), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY12), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY12), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY17), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY17), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY18), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY18), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    SetWindowLong(GetDlgItem(hwTB2, IDC_STRY19), GWL_STYLE, WS_BORDER | WS_CHILD | WS_VISIBLE | SS_BLACKRECT);
    SetWindowPos(GetDlgItem(hwTB2, IDC_STRY19), 0, 0, 0, 5, 100, SWP_NOMOVE | SWP_NOZORDER);
    if (UndoAvailable > 0) EnableWindow(GetDlgItem(hwTB2, IDC_UNDO), TRUE); else EnableWindow(GetDlgItem(hwTB2, IDC_UNDO), FALSE);
    if (RedoAvailable > 0) EnableWindow(GetDlgItem(hwTB2, IDC_REDO), TRUE); else EnableWindow(GetDlgItem(hwTB2, IDC_REDO), FALSE);
    /*oldSpriteListFunc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwTB2, IDC_SPRITELIST), GWLP_WNDPROC);
    SetWindowSubclass(GetDlgItem(hwTB2, IDC_SPRITELIST), (SUBCLASSPROC)SubclassSpriteListProc, 5, 0);
    oldSpriteDetListFunc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwTB2, IDC_DETSPR), GWLP_WNDPROC);
    SetWindowSubclass(GetDlgItem(hwTB2, IDC_DETSPR), (SUBCLASSPROC)SubclassSpriteDetListProc, 6, 0);*/
    char tbuf[8];
    SendMessage(GetDlgItem(hwTB2, IDC_DETSPR), CB_RESETCONTENT, 0, 0);
    for (int ti = 0; ti < MAX_SPRITE_DETECT_AREAS; ti++)
    {
        _itoa_s(ti + 1, tbuf, 8, 10);
        SendMessageA(GetDlgItem(hwTB2, IDC_DETSPR), CB_ADDSTRING, 0, (LPARAM)tbuf);
    }
    SendMessage(GetDlgItem(hwTB2, IDC_DETSPR), CB_SETCURSEL, 0, 1);
    acDetSprite = 0;
    UpdateSpriteList();
    UpdateSpriteList3();
    return true;
}
 
bool CreateTextures(void)
{
    // Create the textures for the lower strip showing the frames
    glfwMakeContextCurrent(glfwframestrip);
    MonWidth = GetSystemMetrics(SM_CXFULLSCREEN);

    pFrameStrip = (UINT8*)malloc(MonWidth * FRAME_STRIP_HEIGHT * 4); //RGBA for efficiency and alignment
    if (!pFrameStrip)
    {
        return false;
    }

    glGenTextures(2, TxFrameStrip);

    glBindTexture(GL_TEXTURE_2D, TxFrameStrip[0]);
    // We allocate a texture corresponding to the width of the screen in case we are fullscreen by 64-pixel (frame) + 2x16-pixel (margins) height
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, MonWidth, FRAME_STRIP_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, TxFrameStrip[1]);
    // We allocate a texture corresponding to the width of the screen in case we are fullscreen by 64-pixel (frame) + 2x16-pixel (margins) height
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, MonWidth, FRAME_STRIP_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Create the number texture
    Bitmap* pbmp = Bitmap::FromFile(L"textures\\chiffres.png");
    if (pbmp == NULL) return false;
    Rect rect = Rect(0, 0, pbmp->GetWidth(), pbmp->GetHeight());
    BitmapData bmpdata;
    pbmp->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpdata);
    glGenTextures(1, &TxChiffres);

    glBindTexture(GL_TEXTURE_2D, TxChiffres);
    // We allocate a texture corresponding to the width of the screen in case we are fullscreen by 64-pixel (frame) + 2x16-pixel (margins) height
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 200, 32, 0, GL_BGRA, GL_UNSIGNED_BYTE, bmpdata.Scan0);
    pbmp->UnlockBits(&bmpdata);
    delete pbmp;

    // Create the textures for the lower strip showing the sprite
    glfwMakeContextCurrent(glfwspritestrip);
    MonWidth = GetSystemMetrics(SM_CXFULLSCREEN);

    pSpriteStrip = (UINT8*)malloc(MonWidth * FRAME_STRIP_HEIGHT2 * 4); //RGBA for efficiency and alignment
    if (!pSpriteStrip)
    {
        return false;
    }

    glGenTextures(2, TxSpriteStrip);

    glBindTexture(GL_TEXTURE_2D, TxSpriteStrip[0]);
    // We allocate a texture corresponding to the width of the screen in case we are fullscreen by 64-pixel (frame) + 2x16-pixel (margins) height
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, MonWidth, FRAME_STRIP_HEIGHT2, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, TxSpriteStrip[1]);
    // We allocate a texture corresponding to the width of the screen in case we are fullscreen by 64-pixel (frame) + 2x16-pixel (margins) height
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, MonWidth, FRAME_STRIP_HEIGHT2, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    // Create the cRom texture
    glfwMakeContextCurrent(glfwframe);
    pbmp = NULL;
    pbmp = Bitmap::FromFile(L"textures\\cRom.png");
    if (pbmp == NULL) return false;
    rect = Rect(0, 0, pbmp->GetWidth(), pbmp->GetHeight());
    pbmp->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bmpdata);
    glGenTextures(1, &TxcRom);

    glBindTexture(GL_TEXTURE_2D, TxcRom);
    // We allocate a texture corresponding to the width of the screen in case we are fullscreen by 64-pixel (frame) + 2x16-pixel (margins) height
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rect.Width, rect.Height, 0, GL_BGRA, GL_UNSIGNED_BYTE, bmpdata.Scan0);
    pbmp->UnlockBits(&bmpdata);
    delete pbmp;

    // Read the RAW digit definition
    FILE* pfile;
    if (fopen_s(&pfile, "textures\\chiffres.raw", "rb")) return false;
    fread(Raw_Digit_Def, 1, RAW_DIGIT_W * RAW_DIGIT_H * 10, pfile);
    fclose(pfile);
    return true;
}

bool glfwCreateCursorFromFile(char* rawfile,GLFWcursor** pcursor)
{
    unsigned char pdata[32 * 32 * 4];
    FILE* pf;
    if (fopen_s(&pf, rawfile, "rb")) return false;
    rewind(pf);
    fread(pdata, 1, 4 * 32 * 32, pf);
    fclose(pf);
    GLFWimage image;
    image.height = image.width = 32;
    image.pixels = pdata;
    *pcursor = glfwCreateCursor(&image, 0, 0);
    return true;
}

HWND DoCreateStatusBar(HWND hwndParent, HINSTANCE  hinst)
{
    HWND hwndStatus;
    RECT rcClient;
    HLOCAL hloc;
    PINT paParts;
    int nWidth;

    // Ensure that the common control DLL is loaded.
    InitCommonControls();

    // Create the status bar.
    hwndStatus = CreateWindowEx(
        0,                       // no extended styles
        STATUSCLASSNAME,         // name of status bar class
        (PCTSTR)NULL,           // no text when first created
        WS_CHILD | WS_VISIBLE,   // creates a visible child window
        0, 0, 0, 0,              // ignores size and position
        hwndParent,              // handle to parent window
        NULL,       // child window identifier
        hinst,                   // handle to application instance
        NULL);                   // no window creation data

    // Get the coordinates of the parent window's client area.
    GetClientRect(hwndParent, &rcClient);

    // Allocate an array for holding the right edge coordinates.
    hloc = LocalAlloc(LHND, sizeof(int));
    paParts = (PINT)LocalLock(hloc);

    // Calculate the right edge coordinate for each part, and
    // copy the coordinates to the array.
    nWidth = rcClient.right;
    int rightEdge = nWidth;
    paParts[0] = rightEdge;
    rightEdge += nWidth;

    // Tell the status bar to create the window parts.
    SendMessage(hwndStatus, SB_SETPARTS, 0, (LPARAM)paParts);

    // Free the array, and return.
    LocalUnlock(hloc);
    LocalFree(hloc);
    statusBarHeight = GetSystemMetrics(SM_CYCAPTION) + 10;
    return hwndStatus;
}  

#pragma endregion Window_Creations

#pragma region Main

bool isLeftReleased = false, isRightReleased = false;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    bool mselcolup = true;
    mselcol = 0;

    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

    hInst = hInstance;

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_UPDOWN_CLASS;
    InitCommonControlsEx(&icex);   
    CoInitialize(NULL);
    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCEL));

    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    if (GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL) != 0)
    {
        cprintf("Can't initialize GDI+");
        return -1;
    }
    // create console and disable the close button
    AllocConsole();
    HWND thdl = GetConsoleWindow();
    HMENU thmen= GetSystemMenu(thdl, FALSE);
    DeleteMenu(thmen, SC_CLOSE, MF_BYCOMMAND);
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    cprintf("ColorizingDMD started");
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_CROM)); // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_COLORIZINGDMD));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;// MAKEINTRESOURCEW(IDC_COLORIZINGDMD);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = NULL; // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    if (!RegisterClassEx(&wcex))
    {
        cprintf("Call to RegisterClassEx failed!");
        return 1;
    }
    WNDCLASSEXW wcex2;
    wcex2.cbSize = sizeof(WNDCLASSEX);
    wcex2.style = CS_HREDRAW | CS_VREDRAW;
    wcex2.lpfnWndProc = WndProc2;
    wcex2.cbClsExtra = 0;
    wcex2.cbWndExtra = 0;
    wcex2.hInstance = hInstance;
    wcex2.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_CROM)); // LoadIcon(hInstance, MAKEINTRESOURCE(IDI_COLORIZINGDMD));
    wcex2.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex2.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex2.lpszMenuName = NULL;// MAKEINTRESOURCEW(IDC_COLORIZINGDMD);
    wcex2.lpszClassName = szWindowClass2;
    wcex2.hIconSm = NULL; // LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    if (!RegisterClassEx(&wcex2))
    {
        cprintf("Call to RegisterClassEx2 failed!");
        return 1;
    }
    hWnd = CreateWindow(szWindowClass, L"ColorizingDMD", WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1480,900, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd)
    {
        AffLastError((char*)"CreateWindow");
        return FALSE;
    }
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    hStatus=DoCreateStatusBar(hWnd, hInst);
    hSprites = CreateWindow(szWindowClass2, L"Sprites", WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 1480,900, nullptr, nullptr, hInstance, nullptr);
    if (!hSprites)
    {
        AffLastError((char*)"CreateWindow2");
        return FALSE;
    }
    ShowWindow(hSprites, SW_SHOW);
    UpdateWindow(hSprites);
    hStatus2 = DoCreateStatusBar(hSprites, hInst);
    LoadWindowPosition();

    WNDCLASSEX child;
    child.cbSize = sizeof(WNDCLASSEX);
    child.style = 0;
    child.lpfnWndProc = PalProc;
    child.cbClsExtra = 0;
    child.cbWndExtra = 0;
    child.hInstance = hInstance;
    child.hIcon = NULL;
    child.hCursor = NULL;
    child.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    child.lpszMenuName = NULL;
    child.lpszClassName = L"Palette";
    child.hIconSm = NULL;
    if (!RegisterClassEx(&child))
    {
        cprintf("Call to RegisterClassEx for palette failed!");
        return 1;
    }
    child.lpfnWndProc = MovSecProc;
    child.lpszClassName = L"MovSection";
    if (!RegisterClassEx(&child))
    {
        cprintf("Call to RegisterClassEx for moving sections failed!");
        return 1;
    }

    if (!gl33_InitWindow(&glfwframe, 10, 10, "Frame", hWnd)) return -1;
    if (!gl33_InitWindow(&glfwframestrip, 10, 10, "Frame Strip", hWnd)) return -1;
    if (!gl33_InitWindow(&glfwsprites, 10, 10, "Sprite", hSprites)) return -1;
    if (!gl33_InitWindow(&glfwspritestrip, 10, 10, "Sprite Strip", hSprites)) return -1;
    glfwSetMouseButtonCallback(glfwframe, mouse_button_callback);
    glfwSetMouseButtonCallback(glfwframestrip, mouse_button_callback);
    glfwSetCursorPosCallback(glfwframe, mouse_move_callback);
    glfwSetCursorPosCallback(glfwframestrip, mouse_move_callback);
    glfwSetScrollCallback(glfwframe, mouse_scroll_callback);
    glfwSetScrollCallback(glfwframestrip, mouse_scroll_callback);
    glfwSetMouseButtonCallback(glfwsprites, mouse_button_callback2);
    glfwSetMouseButtonCallback(glfwspritestrip, mouse_button_callback2);
    glfwSetCursorPosCallback(glfwsprites, mouse_move_callback2);
    glfwSetCursorPosCallback(glfwspritestrip, mouse_move_callback2);
    glfwSetScrollCallback(glfwsprites, mouse_scroll_callback2);
    glfwSetScrollCallback(glfwspritestrip, mouse_scroll_callback2);

    LoadSaveDir();

    if (!CreateToolbar()) return -1;
    if (!CreateToolbar2()) return -1;
    TxCircle=text_CreateTextureCircle(glfwframe);
    MSG msg;

    // allocate space for undo/redo
    UndoSave = (UINT8*)malloc(MAX_SEL_FRAMES * 256 * 64 * MAX_UNDO);
    RedoSave = (UINT8*)malloc(MAX_SEL_FRAMES * 256 * 64 * MAX_UNDO);
    if ((!UndoSave) || (!RedoSave))
    {
        cprintf("Can't get the memory for undo/redo actions");
        return -1;
    }

    hcArrow = LoadCursor(nullptr, IDC_ARROW);
    hcColPick = LoadCursor(hInst, MAKEINTRESOURCE(IDC_NODROP));
    hcPaste = LoadCursor(hInst, MAKEINTRESOURCE(IDC_PASTE));
    glfwCreateCursorFromFile((char*)"paste.raw", &glfwpastecur);
    glfwCreateCursorFromFile((char*)"NODROP.raw", &glfwdropcur);
    glfwarrowcur = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    
    if (!CreateTextures()) return -1;
    Calc_Resize_Frame();
    Calc_Resize_Sprite();
    UpdateFSneeded = true;
    timeSelFrame = timeGetTime() + 500;
    DWORD tickCount = GetTickCount();
    build_crc32_table(); // prepare CRC32 calculations

    // Boucle de messages principale :
    while (!fDone)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        if (MycRom.name[0] != 0)
        {
            DWORD acCount = GetTickCount();
            if ((acCount < tickCount) || (acCount - tickCount > AUTOSAVE_TICKS))
            {
                if (Save_cRom(true))
                {
                    cprintf("%s.cROM autosaved in %s", MycRom.name, MycRP.SaveDir);
                    if (Save_cRP(true)) cprintf("%s.cRP autosaved in %s", MycRom.name, MycRP.SaveDir);
                }
                tickCount = acCount;
            }
        }
        if (Update_Toolbar)
        {
            CreateToolbar();
            Update_Toolbar = false;
        }
        if (Update_Toolbar2)
        {
            CreateToolbar2();
            Update_Toolbar2 = false;
        }
        if (mselcolup)
        {
            mselcol+=3;
            if (mselcol > 250) mselcolup = false;
        }
        else
        {
            mselcol -= 3;
            if (mselcol < 5) mselcolup = true;
        }
        if (!IsIconic(hWnd))
        {
            if ((MycRom.cPal) && (timeGetTime() > timeSelFrame))
            {
                timeSelFrame = timeGetTime() + 500;
                if (SelFrameColor == 0) SelFrameColor = 255; else SelFrameColor = 0;
                if (hPal) InvalidateRect(hPal, NULL, FALSE);
                if (hPal2) InvalidateRect(hPal2, NULL, FALSE);
                if (hPal3) InvalidateRect(hPal3, NULL, FALSE);
                if ((Edit_Mode == 1)&&(hwTB)) InvalidateRect(GetDlgItem(hwTB, IDC_GRADMODEB), NULL, FALSE);
            }
            if (!(GetKeyState(VK_LEFT) & 0x8000)) isLeftReleased = true;
            if (!(GetKeyState(VK_RIGHT) & 0x8000)) isRightReleased = true;
            if (GetForegroundWindow() == hWnd)
            {
                if (isPressed(VK_LEFT, &timeLPress))
                {
                    if ((GetKeyState(VK_CONTROL) & 0x8000) && (nSameFrames > 0))
                    {
                        int acsame = -1;
                        for (UINT ti = 0; ti < (UINT)nSameFrames; ti++)
                        {
                            if ((SameFrames[ti] < PreFrameInStrip) && (SameFrames[ti] > acsame)) acsame = SameFrames[ti];
                        }
                        if (acsame >= 0)
                        {
                            PreFrameInStrip = acsame;
                            UpdateFSneeded = true;
                        }
                    }
                    else if ((GetKeyState(VK_SHIFT) & 0x8000) && (nSelFrames > 1))
                    {
                        int acsel = -1;
                        for (UINT ti = 0; ti < nSelFrames; ti++)
                        {
                            if ((SelFrames[ti] < (UINT)PreFrameInStrip) && ((int)SelFrames[ti] > acsel)) acsel = (int)SelFrames[ti];
                        }
                        if (acsel >= 0)
                        {
                            PreFrameInStrip = acsel;
                            UpdateFSneeded = true;
                        }
                    }
                    else if ((!(GetKeyState(VK_CONTROL) & 0x8000)) && (!(GetKeyState(VK_SHIFT) & 0x8000)) && (acFrame > 0))
                    {
                        if (isLeftReleased)
                        {
                            SaveAction(true, SA_SELECTION);
                            isLeftReleased = false;
                        }
                        acFrame--;
                        InitColorRotation();
                        if (!isFrameSelected2(acFrame))
                        {
                            nSelFrames = 1;
                            SelFrames[0] = acFrame;
                        }
                        if (acFrame >= PreFrameInStrip + NFrameToDraw) PreFrameInStrip = acFrame - NFrameToDraw + 1;
                        if ((int)acFrame < PreFrameInStrip) PreFrameInStrip = acFrame;
                        UpdateFSneeded = true;
                        UpdateNewacFrame();
                    }
                }
                if (isPressed(VK_RIGHT, &timeRPress))
                {
                    if ((GetKeyState(VK_CONTROL) & 0x8000) && (nSameFrames > 0))
                    {
                        int acsame = 2000000000;
                        for (UINT ti = 0; ti < (UINT)nSameFrames; ti++)
                        {
                            if ((SameFrames[ti] > PreFrameInStrip) && (SameFrames[ti] < acsame)) acsame = SameFrames[ti];
                        }
                        if (acsame < 2000000000)
                        {
                            PreFrameInStrip = acsame;
                            UpdateFSneeded = true;
                        }
                    }
                    else if ((GetKeyState(VK_SHIFT) & 0x8000) && (nSelFrames > 1))
                    {
                        int acsel = 2000000000;
                        for (UINT ti = 0; ti < nSelFrames; ti++)
                        {
                            if ((SelFrames[ti] > (UINT)PreFrameInStrip) && ((int)SelFrames[ti] < acsel)) acsel = (int)SelFrames[ti];
                        }
                        if (acsel < 2000000000)
                        {
                            PreFrameInStrip = acsel;
                            UpdateFSneeded = true;
                        }
                    }
                    else if ((!(GetKeyState(VK_CONTROL) & 0x8000)) && (!(GetKeyState(VK_SHIFT) & 0x8000)) && (acFrame < MycRom.nFrames - 1))
                    {
                        if (isRightReleased)
                        {
                            SaveAction(true, SA_SELECTION);
                            isRightReleased = false;
                        }
                        acFrame++;
                        InitColorRotation();
                        if (!isFrameSelected2(acFrame))
                        {
                            nSelFrames = 1;
                            SelFrames[0] = acFrame;
                        }
                        if (acFrame >= PreFrameInStrip + NFrameToDraw) PreFrameInStrip = acFrame - NFrameToDraw + 1;
                        if ((int)acFrame < PreFrameInStrip) PreFrameInStrip = acFrame;
                        UpdateFSneeded = true;
                        UpdateNewacFrame();
                    }
                }
            }
            if (MycRom.name[0] != 0)
            {
                if (Edit_Mode == 1) Draw_Frame(glfwframe, frame_zoom, acFrame, 0, 0, 0, 0, true, false);
                else Draw_Frame(glfwframe, frame_zoom, acFrame, 0, 0, 0, 0, true, true);
            }
            if ((MycRP.name[0] != 0) && (GetForegroundWindow() == hWnd))
            {
                // Setup display to/from surfaces for the different masks and drawing
                EmptyExtraSurface();
                glfwMakeContextCurrent(glfwframe);
                switch (Mouse_Mode)
                {
                    case 0: // no active mode
                    {
                        if (Edit_Mode == 0)
                        {
                            Draw_Extra_Surface[MouseFinPosx + MouseFinPosy * MycRom.fWidth] = 1;
                            SetRenderDrawColor(mselcol, mselcol, mselcol, 255);
                            Draw_Over_From_Surface(Draw_Extra_Surface, 0, (float)frame_zoom, true, true);
                        }
                        else
                        {

                        }
                        break;
                    }
                    case 1: // comparison mask (point)
                    {
                        Draw_Extra_Surface[MouseFinPosx + MouseFinPosy * MycRom.fWidth] = 1;
                        SetRenderDrawColor(mselcol, mselcol, mselcol, 255);
                        Draw_Over_From_Surface(Draw_Extra_Surface, 0, (float)frame_zoom, true, true);
                        break;
                    }
                    case 2: // comparison mask (rect and magic wand)
                    {
                        switch (MycRP.Mask_Sel_Mode)
                        {
                            case 1:
                                drawrectangle(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1, TRUE, false, NULL);
                                break;
                            case 2:
                                drawfill2(MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1);
                                break;
                        }
                        SetRenderDrawColor(mselcol, mselcol, mselcol, 255);
                        Draw_Over_From_Surface(Draw_Extra_Surface, 0, (float)frame_zoom, true, true);
                        break;
                    }
                    case 3: // colorizing the frame or selecting for copying (point)
                    {
                        if (!Copy_Mode)
                        {
                            Draw_Extra_Surface[MouseFinPosx + MouseFinPosy * MycRom.fWidth] = 1;
                            SetRenderDrawColor(mselcol, mselcol, mselcol, 255);
                            Draw_Over_From_Surface(Draw_Extra_Surface, 0, (float)frame_zoom, true, true);
                        }
                        break;
                    }
                    case 4: // colorizing the frame or selecting for copying (others)
                    {
                        switch (MycRP.Draw_Mode)
                        {
                            case 1:
                            {
                                drawline(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1, false, NULL);
                                break;
                            }
                            case 2:
                            {
                                drawrectangle(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1, MycRP.Fill_Mode, false, NULL);
                                break;
                            }
                            case 3:
                            {
                                drawcircle(MouseIniPosx, MouseIniPosy, (int)sqrt((MouseFinPosx - MouseIniPosx)* (MouseFinPosx - MouseIniPosx) + (MouseFinPosy - MouseIniPosy) * (MouseFinPosy - MouseIniPosy)), Draw_Extra_Surface, 1, MycRP.Fill_Mode, false, NULL);
                                break;
                            }
                            case 4:
                            {
                                drawfill(MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1);
                                break;
                            }
                        }
                        if (!Copy_Mode) SetRenderDrawColor(mselcol, mselcol, mselcol, mselcol);
                        else SetRenderDrawColor(mselcol, 0, 0, mselcol);
                        Draw_Over_From_Surface(Draw_Extra_Surface, 0, (float)frame_zoom, true, true);
                        break;
                    }
                    case 5: // drawing dynamic mask (point)
                    {
                        Draw_Extra_Surface[MouseFinPosx + MouseFinPosy * MycRom.fWidth] = 1;
                        SetRenderDrawColor(mselcol, 0, mselcol, 255);
                        Draw_Over_From_Surface(Draw_Extra_Surface, 0, (float)frame_zoom, true, true);
                        break;
                    }
                    case 6: // drawing dynamic mask (others)
                    {
                        switch (MycRP.Draw_Mode)
                        {
                            case 1:
                            {
                                drawline(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1, false, NULL);
                                break;
                            }
                            case 2:
                            {
                                drawrectangle(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1, MycRP.Fill_Mode, false, NULL);
                                break;
                            }
                            case 3:
                            {
                                drawcircle(MouseIniPosx, MouseIniPosy, (int)sqrt((MouseFinPosx - MouseIniPosx) * (MouseFinPosx - MouseIniPosx) + (MouseFinPosy - MouseIniPosy) * (MouseFinPosy - MouseIniPosy)), Draw_Extra_Surface, 1, MycRP.Fill_Mode, false, NULL);
                                break;
                            }
                            case 4:
                            {
                                drawfill(MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1);
                                break;
                            }
                        }
                        SetRenderDrawColor(mselcol, 0, mselcol, 255);
                        Draw_Over_From_Surface(Draw_Extra_Surface, 0, (float)frame_zoom, true, true);
                        break;
                    }
                    case 7: // drawing the copy mask (point)
                    {
                        if (!(GetKeyState('W') & 0x8000) || (MycRom.cFrames[(acFrame * MycRom.fHeight + MouseFinPosy) * MycRom.fWidth + MouseFinPosx] != 0))
                            Draw_Extra_Surface[MouseFinPosx + MouseFinPosy * MycRom.fWidth] = 1;
                        SetRenderDrawColor(0, mselcol, mselcol, 255);
                        Draw_Over_From_Surface(Draw_Extra_Surface, 0, (float)frame_zoom, true, true);
                        break;
                    }
                    case 8: // drawing the copy mask (others)
                    {
                        bool shapeselect = false;
                        if (GetKeyState('W') & 0x8000) shapeselect = true;
                        switch (MycRP.Draw_Mode)
                        {
                            case 1:
                            {
                                drawline(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1, shapeselect, &MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight]);
                                break;
                            }
                            case 2:
                            {
                                drawrectangle(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1, MycRP.Fill_Mode, shapeselect, &MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight]);
                                break;
                            }
                            case 3:
                            {
                                drawcircle(MouseIniPosx, MouseIniPosy, (int)sqrt((MouseFinPosx - MouseIniPosx)* (MouseFinPosx - MouseIniPosx) + (MouseFinPosy - MouseIniPosy) * (MouseFinPosy - MouseIniPosy)), Draw_Extra_Surface, 1, MycRP.Fill_Mode, shapeselect, &MycRom.cFrames[acFrame * MycRom.fWidth * MycRom.fHeight]);
                                break;
                            }
                            case 4:
                            {
                                drawfill(MouseFinPosx, MouseFinPosy, Draw_Extra_Surface, 1);
                                break;
                            }
                        }
                        SetRenderDrawColor(0, mselcol, mselcol, 255);
                        Draw_Over_From_Surface(Draw_Extra_Surface, 0, (float)frame_zoom, true, true);
                        break;
                    }
                }
            }
            if (GetKeyState(VK_ESCAPE) & 0x8000)
            {
                Mouse_Mode = 0;
                Paste_Mode = false;
                Color_Pipette = false;
                glfwSetCursor(glfwframe, glfwarrowcur);
                SetCursor(hcArrow);
            }
            gl33_SwapBuffers(glfwframe, false);
            if (MycRom.name[0] != 0)
            {
                if (UpdateFSneeded)
                {
                    Frame_Strip_Update();
                    UpdateTriggerID();
                    //UpdateFSneededwhenBreleased = false;
                    UpdateFSneeded = false;
                }
                Draw_Frame_Strip();
            }
            if (MouseFrSliderLPressed)
            {
                double x, y;
                glfwGetCursorPos(glfwframe, &x, &y);
                int px = (int)x;// - 2 * FRAME_STRIP_SLIDER_MARGIN;
                if (px != MouseFrSliderlx)
                {
                    MouseFrSliderlx = px;
                    if (px < 0) px = 0;
                    if (px > (int)ScrW - 2 * FRAME_STRIP_SLIDER_MARGIN) px = (int)ScrW - 2 * FRAME_STRIP_SLIDER_MARGIN;
                    PreFrameInStrip = (int)((float)px * (float)MycRom.nFrames / (float)SliderWidth);
                    if (PreFrameInStrip < 0) PreFrameInStrip = 0;
                    if (PreFrameInStrip >= (int)MycRom.nFrames) PreFrameInStrip = (int)MycRom.nFrames - 1;
                    UpdateFSneeded = true;
                }
            }
            float fps = 0;
            fps = gl33_SwapBuffers(glfwframestrip, true);
            CheckAccelerators();
            char tbuf[256];
            /*RECT winrect;
            GetClientRect(hWnd, &winrect);*/
            POINT tpt;
            GetCursorPos(&tpt);
            if (((Mouse_Mode == 2) && (MycRP.Mask_Sel_Mode == 1)) || ((Mouse_Mode == 4) && (MycRP.Draw_Mode == 2)) ||
                ((Mouse_Mode == 6) && (MycRP.Draw_Mode == 2)) || ((Mouse_Mode == 8) && (MycRP.Draw_Mode == 2)))
                sprintf_s(tbuf, 256, "ColorizingDMD v%i.%i.%i (by Zedrummer)     ROM name: %s      Frames: current %i / %i selected / %i total      Pos: (%i,%i)->(%i,%i)      @%.1fFPS", MAJ_VERSION, MIN_VERSION, PATCH_VERSION, MycRom.name, acFrame, nSelFrames, MycRom.nFrames, MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, fps);
            else
                sprintf_s(tbuf, 256, "ColorizingDMD v%i.%i.%i (by Zedrummer)     ROM name: %s      Frames: current %i / %i selected / %i total      Pos: (%i,%i)      @%.1fFPS", MAJ_VERSION, MIN_VERSION, PATCH_VERSION, MycRom.name, acFrame, nSelFrames, MycRom.nFrames, MouseFinPosx, MouseFinPosy, fps);
            SetWindowTextA(hWnd, tbuf);
            glfwPollEvents();
        }
        if (!IsIconic(hSprites))
        {
            Draw_Sprite();
            if (GetForegroundWindow() == hSprites)
            {
                if ((MycRom.name[0] != 0) && (MycRom.nSprites > 0))
                {
                    if (isPressed(VK_LEFT, &timeLPress))
                    {
                        if (acSprite > 0) acSprite--;
                        if (acSprite >= PreSpriteInStrip + NSpriteToDraw) PreSpriteInStrip = acSprite - NSpriteToDraw + 1;
                        if ((int)acSprite < PreSpriteInStrip) PreSpriteInStrip = acSprite;
                        UpdateSSneeded = true;
                    }
                    if (isPressed(VK_RIGHT, &timeRPress))
                    {
                        if (acSprite < MycRom.nSprites - 1) acSprite++;
                        if (acSprite >= PreSpriteInStrip + NSpriteToDraw) PreSpriteInStrip = acSprite - NSpriteToDraw + 1;
                        if ((int)acSprite < PreSpriteInStrip) PreSpriteInStrip = acSprite;
                        UpdateSSneeded = true;
                    }
                    glfwMakeContextCurrent(glfwsprites);
                    EmptyExtraSurface2();
                    if (Mouse_Mode == 4)
                    {
                        switch (Sprite_Mode)
                        {
                        case 1:
                        {
                            drawline2(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface2, 1);
                            break;
                        }
                        case 2:
                        {
                            drawrectangle2(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface2, 1, SpriteFill_Mode);
                            break;
                        }
                        case 3:
                        {
                            drawcircle2(MouseIniPosx, MouseIniPosy, (int)sqrt((MouseFinPosx - MouseIniPosx) * (MouseFinPosx - MouseIniPosx) + (MouseFinPosy - MouseIniPosy) * (MouseFinPosy - MouseIniPosy)), Draw_Extra_Surface2, 1, SpriteFill_Mode);
                            break;
                        }
                        case 4:
                        {
                            drawfill3(MouseFinPosx, MouseFinPosy, Draw_Extra_Surface2, 1);
                            break;
                        }
                        }
                        SetRenderDrawColor(mselcol, mselcol, mselcol, mselcol);
                        Draw_Over_From_Surface2(Draw_Extra_Surface2, 0, (float)sprite_zoom, true, true, false);
                    }
                    else if (Mouse_Mode == 2)
                    {
                        drawrectangle2(MouseIniPosx, MouseIniPosy, MouseFinPosx, MouseFinPosy, Draw_Extra_Surface2, 1, TRUE);
                        SetRenderDrawColor(mselcol, 0, mselcol, mselcol);
                        Draw_Over_From_Surface2(Draw_Extra_Surface2, 0, (float)sprite_zoom, true, true, true);
                    }
                }
            }
            if (UpdateSSneeded)
            {
                Sprite_Strip_Update();
                UpdateSSneeded = false;
            }
            Draw_Sprite_Strip();
            gl33_SwapBuffers(glfwsprites, true);
            gl33_SwapBuffers(glfwspritestrip, true);
            CheckAccelerators();
        }
    }
    SaveSaveDir();
    free(RedoSave);
    free(UndoSave);
    if (pFrameStrip) free(pFrameStrip);
    GdiplusShutdown(gdiplusToken);
    glfwTerminate();
    cprintf("ColorizingDMD terminated");
    return 0;
}

#pragma endregion Main
