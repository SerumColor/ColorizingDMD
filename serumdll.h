#pragma once

#include "serum.h"
#include <Windows.h>

void cprintf(bool isFlash, const char* format, ...);
bool InitLibSerum(char* vpdir, char* fname);
void StopLibSerum(void);
bool ColorizeAFrame(UINT8* vpframe, UINT8** oldframe, UINT16** newframe32, UINT16** newframe64, UINT* frameID, bool* is32fr, bool* is64fr);
bool ColorRotateAFrame(UINT8** oldframe, UINT16** newframe32, UINT8** modelt32, UINT16** newframe64, UINT8** modelt64);
