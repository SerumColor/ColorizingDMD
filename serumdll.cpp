#include "serumdll.h"

//UINT8 isNewFormat = 0; // is the file a new Serum 2+ version (1) or a former version one (0)?
UINT8* ModifiedElements32 = NULL; // for the new color rotations in 32P, optional
UINT8* ModifiedElements64 = NULL; // for the new color rotations in 64P, optional
//UINT triggerID; // return PuP pack trigger ID (0xffff if no trigger)
//UINT8 returnflag; // what frame resolutions are returned with new format colorization
//UINT32 noColors; // number of colors of the original ROM (4 or 16)
UINT32 fWidth, fHeight; // dimensions of the original ROM (MUST BE KNOWN BEFORE calling Serum functions)
//UINT width32 = 0, width64 = 0; // widths of the colorized frames returned respectively for the height=32 and height=64 frames
//UINT ntriggers = 0; // number of PuP triggers found in the file
Serum_Frame_Struc* pSerum;

bool Allocate_Serum(void)
{
    // ---------- Both ModifiedElementsXX are optional so this code may be skipped ----------
    // They are only needed if you only want to change the modified pixels of a frame after a color rotation
    ModifiedElements32 = (UINT8*)malloc(pSerum->width32 * 32);
    ModifiedElements64 = (UINT8*)malloc(pSerum->width64 * 64);
    if (!ModifiedElements32 || !ModifiedElements64)
    {
        cprintf(false, "Can't get memory for the modified pixels in the DLL");
        Serum_Dispose();
        return false;
    }
    return true;
}

bool InitLibSerum(char* vpdir, char* fname)
{
    char tbuf[MAX_PATH];
    strcpy_s(tbuf, MAX_PATH, vpdir);
    if (tbuf[strlen(tbuf) - 1] != '\\' && tbuf[strlen(tbuf) - 1] != '/') strcat_s(tbuf, MAX_PATH, "\\");
    strcat_s(tbuf, MAX_PATH, "altcolor\\");
    pSerum = Serum_Load(tbuf, fname, FLAG_REQUEST_32P_FRAMES | FLAG_REQUEST_64P_FRAMES);
    if (!pSerum)
    {
        cprintf(false, "Serum DLL can't load the file, check the directory");
        FreeLibrary(hSerumDLL);
        return false;
    }
    if (!Allocate_Serum()) return false;
    return true;
}

void UpscaleRGB565Frame(UINT16* frame32, UINT16* frame64, UINT width32)
{
    for (UINT tj = 0; tj < 32; tj++)
    {
        for (UINT ti = 0; ti < width32; ti++)
        {
            UINT16 val = frame32[ti + tj * width32];
            frame64[ti * 2 + (tj * 2) * width32 * 2] = frame64[ti * 2 + 1 + (tj * 2) * width32 * 2] = 
            frame64[ti * 2 + (tj * 2 + 1) * width32 * 2] = frame64[ti * 2 + 1 + (tj * 2 + 1) * width32 * 2] = val;
        }
    }
}

void DownscaleRGB565Frame(UINT16* frame64, UINT16* frame32, UINT width64)
{
    for (UINT tj = 0; tj < 32; tj++)
    {
        for (UINT ti = 0; ti < width64 / 2; ti++)
        {
            frame32[ti + tj * width64 / 2] = frame64[ti * 2 + (tj * 2) * width64];
        }
    }
}

/// <summary>
/// colorize a frame (in return, buffers are available if non NULL)
/// </summary>
/// <param name="vpframe">inbound frame</param>
/// <param name="oldframe">colorized frame if old format</param>
/// <param name="newframe32">colorized frame in 32P if new format</param>
/// <param name="newframe64">colorized frame in 32P if new format</param>
/// <returns>return true if there is an active color rotation, false if not</returns>
bool ColorizeAFrame(UINT8* vpframe, UINT16** newframe32, UINT16** newframe64, UINT* frameID, bool* is32fr, bool* is64fr)
{
        Serum_Colorize(vpframe);
        *frameID = pSerum->frameID;
        *newframe32 = pSerum->frame32;
        *newframe64 = pSerum->frame64;
        if (pSerum->width32 > 0) *is32fr = true; else *is32fr = false;
        if (pSerum->width64 > 0) *is64fr = true; else *is64fr = false;
        for (int ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
        {
            if (pSerum->rotations32[MAX_LENGTH_COLOR_ROTATION * ti] != 0) return true;
            if (pSerum->rotations64[MAX_LENGTH_COLOR_ROTATION * ti] != 0) return true;
        }
    return false;
}

bool ColorRotateAFrame(UINT16** newframe32, UINT8** modelt32, UINT16** newframe64, UINT8** modelt64)
{
    UINT isrot = Serum_Rotate(); // if you don't need them replace ModifiedElementsXX by NULL
    *newframe32 = pSerum->frame32;
    *newframe64 = pSerum->frame64;
    if (modelt32) *modelt32 = ModifiedElements32;
    if (modelt64) *modelt64 = ModifiedElements64;
    if (isrot > 0) return true;
    return false;
}

void StopLibSerum(void)
{
    Serum_Dispose();
}

