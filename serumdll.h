#pragma once

#include "serumwin.h"
#include <Windows.h>

void cprintf(bool isFlash, const char* format, ...);
bool InitLibSerum(char* vpdir, char* fname);
void StopLibSerum(void);
bool ColorizeAFrame(UINT8* vpframe, UINT16** newframe32, UINT16** newframe64, UINT* frameID, bool* is32fr, bool* is64fr);
bool ColorRotateAFrame(UINT16** newframe32, UINT8** modelt32, UINT16** newframe64, UINT8** modelt64);
void UpscaleRGB565Frame(UINT16* frame32, UINT16* frame64, UINT width32);
void DownscaleRGB565Frame(UINT16* frame64, UINT16* frame32, UINT width64);
