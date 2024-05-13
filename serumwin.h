#pragma once

#include <windows.h>
#include "serum.h"

extern HINSTANCE hSerumDLL;
extern Serum_LoadFunc Serum_Load;
extern Serum_DisposeFunc Serum_Dispose, Serum_DisableColorization, Serum_EnableColorization;
extern Serum_ColorizeFunc Serum_Colorize;
extern Serum_RotateFunc Serum_Rotate;
extern Serum_GetVersionFunc Serum_GetVersion, Serum_GetMinorVersion;

void Serum_ReleaseDLL(void);

bool Serum_LoadDLL(const char* File_SerumDLL);
