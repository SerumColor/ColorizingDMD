#pragma once
#include "resource.h"
#include "OGL_Immediate_2D.h"

#define FRAME_STRIP_W_MARGIN 15
#define FRAME_STRIP_H_MARGIN 20
#define FRAME_STRIP_HEIGHT (64+2*FRAME_STRIP_H_MARGIN) // height of the frame strip + 2 margins
#define FRAME_STRIP_HEIGHT2 (128+2*FRAME_STRIP_H_MARGIN) // height of the frame strip + 2 margins
#define FRAME_STRIP_SLIDER_MARGIN 10
#define SPRITE_INTERVAL 10
#define DIGIT_TEXTURE_W 20
#define DIGIT_TEXTURE_H 32
#define RAW_DIGIT_W 11
#define RAW_DIGIT_H 15
#define FIRST_KEY_TIMER_INT 700
#define NEXT_KEY_TIMER_INT 50
#define MARGIN_PALETTE 20
#define AUTOSAVE_TICKS 600000 // 10*60*1000 autosave every 10 minutes

void Frame_Strip_Update(void);
LRESULT CALLBACK Wait_Proc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
void UpdateMaskList(void);
void UpdateSectionList(void);
void UpdateSpriteList(void);
void Del_Section_Frame(UINT nofr);
void Del_Selection_Frame(UINT nofr);
void Del_Same_Frame(UINT nofr);
void RenderDrawPoint(GLFWwindow* glfwin, unsigned int x, unsigned int y, unsigned int zoom);
void CheckSameFrames(void);
int isFrameSelected(UINT noFr);
bool isFrameSelected2(UINT noFr);
int Which_Section(UINT nofr);
void UpdateNewacFrame(void);
void UpdateColorRotDur(HWND hwDlg);
bool CreateToolbar(void);
void InitColorRotation(void);
void UpdateMaskList2(void);
void SetSpotButton(bool);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void UpdateFrameSpriteList(void);
void UpdateTriggerID(void);
