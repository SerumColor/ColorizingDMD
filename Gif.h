#pragma once

#include "gifski.h"
#include <windows.h>

typedef gifski* (*GifskiNewFunc)(const GifskiSettings* settings);
typedef GifskiError(*GifskiAddFrameRgbFunc)(gifski* handle, uint32_t frame_number, uint32_t width, uint32_t bytes_per_row, uint32_t height, const unsigned char* pixels, double presentation_timestamp);
typedef GifskiError(*GifskiSetFileOutputFunc)(gifski* handle, const char* destination_path);
typedef GifskiError(*GifskiFinishFunc)(gifski* handle);
GifskiNewFunc gifski_new2;
GifskiAddFrameRgbFunc gifski_add_frame_rgb2;
GifskiSetFileOutputFunc gifski_set_file_output2;
GifskiFinishFunc gifski_finish2;

HINSTANCE hGIFDLL = NULL;

bool Load_GIFski_DLL(void)
{
    hGIFDLL = LoadLibraryA("gifski.dll");
    if (hGIFDLL == NULL)
    {
        // Handle error
        OutputDebugStringA("Unable to open the gifski.dll file");
        return false;
    }
    gifski_new2 = (GifskiNewFunc)GetProcAddress(hGIFDLL, "gifski_new");
    if (gifski_new2 == NULL)
    {
        OutputDebugStringA("Unable to import the gifski_new function");
        FreeLibrary(hGIFDLL);
        return false;
    }
    gifski_set_file_output2 = (GifskiSetFileOutputFunc)GetProcAddress(hGIFDLL, "gifski_set_file_output");
    if (gifski_set_file_output2 == NULL)
    {
        OutputDebugStringA("Unable to import the gifski_set_file_output function");
        FreeLibrary(hGIFDLL);
        return false;
    }
    gifski_add_frame_rgb2 = (GifskiAddFrameRgbFunc)GetProcAddress(hGIFDLL, "gifski_add_frame_rgb");
    if (gifski_add_frame_rgb2 == NULL)
    {
        OutputDebugStringA("Unable to import the gifski_addframe_rgb function");
        FreeLibrary(hGIFDLL);
        return false;
    }
    gifski_finish2 = (GifskiFinishFunc)GetProcAddress(hGIFDLL, "gifski_finish");
    if (gifski_finish2 == NULL)
    {
        OutputDebugStringA("Unable to import the gifski_finish function");
        FreeLibrary(hGIFDLL);
        return false;
    }
    return true;
}

