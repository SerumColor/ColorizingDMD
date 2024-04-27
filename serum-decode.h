#pragma once

#ifdef _MSC_VER
#define SERUM_API extern "C" __declspec(dllexport)
#else
#define SERUM_API extern "C" __attribute__((visibility("default")))
#endif

#define MIN(a, b) ((a) < (b) ? (a) : (b))

#include "serum.h"

SERUM_API bool Serum_LoadFile(const char* const filename, unsigned int* pnocolors, unsigned int* pntriggers, UINT8 flags, UINT* width32, UINT* width64);
SERUM_API bool Serum_Load(const char* const altcolorpath, const char* const romname, unsigned int* pnocolors, unsigned int* pntriggers, UINT8 flags, UINT* width32, UINT* width64, UINT8* newformat);
SERUM_API void Serum_SetIgnoreUnknownFramesTimeout(UINT16 milliseconds);
SERUM_API void Serum_SetMaximumUnknownFramesToSkip(UINT8 maximum);
SERUM_API void Serum_SetStandardPalette(const UINT8* palette, const int bitDepth);
SERUM_API void Serum_Dispose(void);
SERUM_API bool Serum_ColorizeWithMetadata(UINT8* frame, Serum_Frame* poldframe);
SERUM_API bool Serum_ColorizeWithMetadataN(UINT8* frame, Serum_Frame_New* pnewframe);
SERUM_API bool Serum_Colorize(UINT8* frame, Serum_Frame* poldframe, Serum_Frame_New* pnewframe);
SERUM_API bool Serum_ApplyRotations(Serum_Frame* poldframe);// UINT8* palette, UINT8* rotations);
SERUM_API bool Serum_ApplyRotationsN(Serum_Frame_New* pnewframe, UINT8* modelements32, UINT8* modelements64);// UINT16* frame, UINT8* modelements, UINT16* rotationsinframe, UINT sizeframe, UINT16* rotations, bool is32);
/*SERUM_API bool Serum_ColorizeWithMetadataOrApplyRotations(
	UINT8* frame, int width, int height, UINT8* palette, UINT8* rotations,
	UINT32* triggerID, UINT32* hashcode, int* frameID);
SERUM_API bool Serum_ColorizeOrApplyRotations(UINT8* frame, int width,
											  int height, UINT8* palette,
											  UINT32* triggerID);*/
SERUM_API void Serum_DisableColorization(void);
SERUM_API void Serum_EnableColorization(void);
SERUM_API const char* Serum_GetVersion(void);
SERUM_API const char* Serum_GetMinorVersion(void);

