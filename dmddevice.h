#ifndef DMDDEVICE_H
#define DMDDEVICE_H

#include <windows.h>
#include <stdio.h>

typedef enum {
	None,
	_2x16Alpha,
	_2x20Alpha,
	_2x7Alpha_2x7Num,
	_2x7Alpha_2x7Num_4x1Num,
	_2x7Num_2x7Num_4x1Num,
	_2x7Num_2x7Num_10x1Num,
	_2x7Num_2x7Num_4x1Num_gen7,
	_2x7Num10_2x7Num10_4x1Num,
	_2x6Num_2x6Num_4x1Num,
	_2x6Num10_2x6Num10_4x1Num,
	_4x7Num10,
	_6x4Num_4x1Num,
	_2x7Num_4x1Num_1x16Alpha,
	_1x16Alpha_1x16Num_1x7Num
} layout_t;

typedef struct {
	int dmd_red, dmd_green, dmd_blue;
	int dmd_perc66, dmd_perc33, dmd_perc0;
	int dmd_only, dmd_compact, dmd_antialias;
	int dmd_colorize;
	int dmd_red66, dmd_green66, dmd_blue66;
	int dmd_red33, dmd_green33, dmd_blue33;
	int dmd_red0, dmd_green0, dmd_blue0;
	int dmd_opacity;
	int resampling_quality;
	int vgmwrite;
} tPMoptions;

typedef struct rgb24 {
	UINT8 red;
	UINT8 green;
	UINT8 blue;
} rgb24;

typedef int(*DmdDev_Open_t)();
typedef bool(*DmdDev_Close_t)();
typedef void(*DmdDev_PM_GameSettings_t)(const char* GameName, UINT64 HardwareGeneration, const tPMoptions& Options);
typedef void(*DmdDev_Set_4_Colors_Palette_t)(rgb24 color0, rgb24 color33, rgb24 color66, rgb24 color100);
typedef void(*DmdDev_Console_Data_t)(UINT8 data);
typedef int(*DmdDev_Console_Input_t)(UINT8* buf, int size);
typedef int(*DmdDev_Console_Input_Ptr_t)(DmdDev_Console_Input_t ptr);
typedef void(*DmdDev_Render_16_Shades_t)(UINT16 width, UINT16 height, UINT8* currbuffer);
typedef void(*DmdDev_Render_4_Shades_t)(UINT16 width, UINT16 height, UINT8* currbuffer);
typedef void(*DmdDev_Render_16_Shades_with_Raw_t)(UINT16 width, UINT16 height, UINT8* currbuffer, UINT32 noOfRawFrames, UINT8* rawbuffer);
typedef void(*DmdDev_Render_4_Shades_with_Raw_t)(UINT16 width, UINT16 height, UINT8* currbuffer, UINT32 noOfRawFrames, UINT8* rawbuffer);
typedef void(*DmdDev_render_PM_Alphanumeric_Frame_t)(layout_t layout, const UINT16* const seg_data, const UINT16* const seg_data2);
typedef void(*DmdDev_render_PM_Alphanumeric_Dim_Frame_t)(layout_t layout, const UINT16* const seg_data, const char* const seg_dim, const UINT16* const seg_data2);


const char* InitDmdDevice(char* VPpath, char* romname);
void StopDmdDevice(void);
void SendFrameToTester(unsigned int nofr, unsigned int nocolors, UINT8* pframes, unsigned int width, unsigned int height, unsigned char* TesterOriginalFrame);
const char* ImportDump(char* path, char* _gamename, UINT8** ppframes, UINT** pptimecodes, UINT* nframes, UINT nocolors, UINT width, UINT height);

#endif	