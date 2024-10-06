#pragma once
#include "resource.h"
#include "OGL_Immediate_2D.h"
#include <string>
#include "ResizeFilters.h"
#include <opencv2/opencv.hpp>
using namespace cv;

#define FRAME_STRIP_W_MARGIN 15
#define FRAME_STRIP_H_MARGIN 20
#define FRAME_STRIP_HEIGHT (64 + 2 * FRAME_STRIP_H_MARGIN) // height of the frame strip + 2 margins
#define FRAME_STRIP_HEIGHT2 (64 + 2 * FRAME_STRIP_H_MARGIN) // height of the sprite strip + 2 margins
#define FRAME_STRIP_SLIDER_MARGIN 10
#define SPRITE_INTERVAL 10
#define DIGIT_TEXTURE_W 20
#define DIGIT_TEXTURE_H 32
#define RAW_DIGIT_W 11
#define RAW_DIGIT_H 15
#define FIRST_KEY_TIMER_INT 700
#define NEXT_KEY_TIMER_INT 50
#define MARGIN_PALETTE_X 20
#define MARGIN_PALETTE_Y 100
#define AUTOSAVE_TICKS 600000 // 10*60*1000 autosave every 10 minutes

void Frame_Strip_Update(void);
LRESULT CALLBACK Wait_Proc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
void UpdateMaskList(void);
void UpdateSectionList(void);
void UpdateSpriteList(void);
void Del_Section_Frame(UINT nofr);
void Del_Selection_Frame(UINT nofr);
void Del_Same_Frame(UINT nofr);
void RenderDrawPoint(GLFWwindow* glfwin, float x, float y, float zoom);
void CheckSameFrames(void);
UINT CheckSameFrames(UINT8 nomsk, BOOL shapemode);
int isFrameSelected(UINT noFr);
bool isFrameSelected2(UINT noFr);
int Which_Section(UINT nofr);
void UpdateNewacFrame(void);
void UpdateColorRotDur(HWND hwDlg);
bool CreateToolbar(void);
void InitColorRotation(void);
void InitColorRotation2(void);
void UpdateMaskList2(void);
void SetSpotButton(bool);
void SetBackgroundMaskSpotButton(bool);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void UpdateFrameSpriteList(void);
void UpdateTriggerID(void);
void UpdatePuPList(void);
bool MaskUsed(UINT32 nomask);
LRESULT CALLBACK Filter_Proc(HWND hwDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
void UpdateSpriteList2(void);
void UpdateSpriteList3(void);
void GetSelectionSize(void);
void mouse_scroll_callback3(GLFWwindow* window, double xoffset, double yoffset);
void SavePaths(void);
bool Save_cRom(bool autosave, bool fastsave, char* forcepath);
void SaveWindowPosition(void);
HWND DoCreateStatusBar(HWND hwndParent, HINSTANCE  hinst);
bool SetIcon(HWND ButHWND, UINT ButIco);
void UpdateFrameBG(void);
void AddBackground(void);
void DeleteBackground(void);
void cprintf(bool isFlash, const char* format, ...); // write to the console
void SetCommonButton(bool ison);
void SetZoomButton(bool ison);
void SetZoomSpriteButton(bool ison);
void UpdateBackgroundList(void);
void Calc_Resize_BG(void);
void SetMultiWarningF(void);
void SetMultiWarningS(void);
void Predraw_BG_For_Rotations(unsigned int noBG);
uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b);
void rgb565_to_rgb888(uint16_t rgb565, uint8_t* r, uint8_t* g, uint8_t* b);
void rgb565_to_rgb888(uint16_t rgb565, uint8_t* rgb888);
void SetExtraResBBox(void);
void Calc_Resize_Frame(void);
void Calc_Resize_Sprite(void);
COLORREF RGB565_to_RGB888(UINT16 rgb565);
void SetBackground(void);
void BubbleSort(UINT* arr, UINT n);
UINT8 ValuePlus2x2(UINT8* pbuf, UINT lineW);
void InitCropZoneList(void);
void ModBackgroundMask(bool AddMask);
void generateAIImage(const std::string& prompt, const std::string api_key, HWND hDlg);
void variationAIImage(const std::string api_key, HWND hDlg);
void DrawImagePix(UINT8* pimage, UINT16* protation, UINT pixnb, UINT width, UINT16 pcol, UINT sizepix, UINT16 colrot, UINT16 nocolrot);

//void (*resizefilterFunc)(UINT16*, UINT16*);
enum
{
	RF_NEAREST,
	RF_BILINEAR,
	RF_BICUBIC,
	RF_LANCZOS4,
	RF_SCALE2X,
	//RF_HQ2X,
	NUMBER_OF_RESIZE_FILTERS
}resize_filters_list;
const char resize_filters_name[NUMBER_OF_RESIZE_FILTERS][16] = { "Nearest", "Bilinear", "Bicubic", "Lanczos4", "Scale2x" };// , "HQ2x" };
const int resize_filters_isCV[NUMBER_OF_RESIZE_FILTERS] = { cv::INTER_NEAREST,cv::INTER_LINEAR,cv::INTER_CUBIC,cv::INTER_LANCZOS4,-1 };// , -1};
//const void* resize_filters_proc[NUMBER_OF_RESIZE_FILTERS] = { NULL,NULL,NULL,NULL,Filter_Scale2X,Filter_HQ2X};
void (*resize_filters_proc[NUMBER_OF_RESIZE_FILTERS])(UINT16*, UINT16*, UINT, UINT) = { NULL,NULL,NULL,NULL,Filter_Scale2X };// , Filter_HQ2X };
void (*resize_filters_mask_proc[NUMBER_OF_RESIZE_FILTERS])(UINT16*, UINT16*, UINT8*, UINT8*, UINT, UINT) = { NULL,NULL,NULL,NULL,Filter_Scale2Xmask };// , Filter_HQ2Xmask };
const UINT8 resize_filters_upordown[NUMBER_OF_RESIZE_FILTERS] = { 3,3,3,3,1 };// , 1 }; // 1: filter to upscale, 2: filter to downscale, 3: filter for doing both
