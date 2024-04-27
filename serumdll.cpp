#include "serumdll.h"

HINSTANCE hSerumDLL;
Serum_LoadFunc serum_Load;
Serum_DisposeFunc serum_Dispose;
Serum_ColorizeFunc serum_Colorize;
Serum_ApplyRotationsFunc serum_ApplyRotations;
Serum_ApplyRotationsNFunc serum_ApplyRotationsN;

UINT8 isNewFormat = 0; // is the file a new Serum 2+ version (1) or a former version one (0)?
Serum_Frame MyOldFrame; // structure to communicate with former format Serum 
Serum_Frame_New MyNewFrame; // structure to communicate with new format Serum 
UINT8* ModifiedElements32 = NULL; // for the new color rotations in 32P, optional
UINT8* ModifiedElements64 = NULL; // for the new color rotations in 64P, optional
UINT triggerID; // return PuP pack trigger ID (0xffff if no trigger)
UINT8 returnflag; // what frame resolutions are returned with new format colorization
UINT32 noColors; // number of colors of the original ROM (4 or 16)
UINT32 fWidth, fHeight; // dimensions of the original ROM (MUST BE KNOWN BEFORE calling Serum functions)
UINT width32 = 0, width64 = 0; // widths of the colorized frames returned respectively for the height=32 and height=64 frames
UINT ntriggers = 0; // number of PuP triggers found in the file

bool Load_Serum_DLL(void)
{
    // Function to load the serum library and all its needed functions, call it in your initial code
    // replace File_SerumDLL by a const char* with the full path and name of the DLL
    // like "c:\\visual pinball\\vpinmame\\serum64.dll"
    char tbuf[MAX_PATH];
    GetModuleFileNameA(NULL, tbuf, MAX_PATH);
    //strcpy_s(tbuf, MAX_PATH, vpdir);
    //if (tbuf[strlen(tbuf) - 1] != '\\' && tbuf[strlen(tbuf) - 1] != '/') strcat_s(tbuf, MAX_PATH, "\\");
    int ti = (int)strlen(tbuf);
    while (tbuf[ti] != '\\' && tbuf[ti] != '/' && ti > 0) ti--;
    if (ti == 0)
    {
        cprintf(true, "Error in getting the module path for serum64.dll", tbuf);
        return false;
    }
    tbuf[ti + 1] = 0;
    strcat_s(tbuf, MAX_PATH, "serum64.dll");
    hSerumDLL = LoadLibraryA(tbuf);
    if (hSerumDLL == NULL)
    {
        cprintf(true, "Can't open %s", tbuf);
        return false;
    }
    serum_Load = (Serum_LoadFunc)GetProcAddress(hSerumDLL, "Serum_Load");
    if (serum_Load == NULL)
    {
        cprintf(true, "Can't find Serum_Load function in the DLL");
        FreeLibrary(hSerumDLL);
        return false;
    }
    serum_Dispose = (Serum_DisposeFunc)GetProcAddress(hSerumDLL, "Serum_Dispose");
    if (serum_Dispose == NULL)
    {
        cprintf(true, "Can't find Serum_Dispose function in the DLL");
        FreeLibrary(hSerumDLL);
        return false;
    }
    serum_Colorize = (Serum_ColorizeFunc)GetProcAddress(hSerumDLL, "Serum_Colorize");
    if (serum_Colorize == NULL)
    {
        cprintf(true, "Can't find Serum_Colorize function in the DLL");
        FreeLibrary(hSerumDLL);
        return false;
    }
    serum_ApplyRotations = (Serum_ApplyRotationsFunc)GetProcAddress(hSerumDLL, "Serum_ApplyRotations");
    if (serum_ApplyRotations == NULL)
    {
        cprintf(true, "Can't find Serum_ApplyRotations function in the DLL");
        FreeLibrary(hSerumDLL);
        return false;
    }
    serum_ApplyRotationsN = (Serum_ApplyRotationsNFunc)GetProcAddress(hSerumDLL, "Serum_ApplyRotationsN");
    if (serum_ApplyRotationsN == NULL)
    {
        cprintf(true, "Can't find Serum_ApplyRotationsN function in the DLL");
        FreeLibrary(hSerumDLL);
        return false;
    }
    return true;
}

void Free_element(void* pElement)
{
    if (pElement)
    {
        free(pElement);
        pElement = NULL;
    }
}

void Free_Serum(void)
{
    Free_element(MyOldFrame.frame);
    Free_element(MyOldFrame.palette);
    Free_element(MyOldFrame.rotations);
    Free_element(MyNewFrame.frame32);
    Free_element(MyNewFrame.frame64);
    Free_element(MyNewFrame.rotations32);
    Free_element(MyNewFrame.rotations64);
    Free_element(MyNewFrame.rotationsinframe32);
    Free_element(MyNewFrame.rotationsinframe64);
    Free_element(ModifiedElements32);
    Free_element(ModifiedElements64);
}

bool Allocate_Serum(void)
{
    MyOldFrame.palette = NULL;
    MyOldFrame.rotations = NULL;
    MyNewFrame.frame32 = NULL;
    MyNewFrame.frame64 = NULL;
    MyNewFrame.rotations32 = NULL;
    MyNewFrame.rotations64 = NULL;
    MyNewFrame.rotationsinframe32 = NULL;
    MyNewFrame.rotationsinframe64 = NULL;
    if (isNewFormat == 0)
    {
        MyOldFrame.frame = (UINT8*)malloc(fWidth * fHeight);
        MyOldFrame.palette = (UINT8*)malloc(3 * 64);
        MyOldFrame.rotations = (UINT8*)malloc(3 * MAX_COLOR_ROTATIONS);
        MyOldFrame.triggerID = &triggerID;
        if (!MyOldFrame.frame || !MyOldFrame.palette || !MyOldFrame.rotations)
        {
            cprintf(false, "Can't get memory for the DLL with old format");
            FreeLibrary(hSerumDLL);
            Free_Serum();
            serum_Dispose();
            return false;
        }
    }
    else
    {
        MyNewFrame.flags = &returnflag;
        MyNewFrame.triggerID = &triggerID;
        // ---------- Both ModifiedElementsXX are optional so this code may be skipped ----------
        // They are only needed if you only want to change the modified pixels of a frame after a color rotation
        ModifiedElements32 = (UINT8*)malloc(width32 * 32);
        ModifiedElements64 = (UINT8*)malloc(width64 * 64);
        if (!ModifiedElements32 || !ModifiedElements64)
        {
            cprintf(false, "Can't get memory for the modified pixels in the DLL");
            FreeLibrary(hSerumDLL);
            Free_Serum();
            serum_Dispose();
            return false;
        }
        // --------------------------------------------------------------------------------------
        if (width32 > 0)
        {
            MyNewFrame.frame32 = (UINT16*)malloc(2 * 32 * width32);
            MyNewFrame.rotations32 = (UINT16*)malloc(2 * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION);
            MyNewFrame.rotationsinframe32 = (UINT16*)malloc(2 * 2 * 32 * width32);
            MyNewFrame.width32 = &width32;
            if (!MyNewFrame.frame32 || !MyNewFrame.rotations32 || !MyNewFrame.rotationsinframe32)
            {
                cprintf(false, "Can't get memory for the 32P elements of the DLL with new format");
                FreeLibrary(hSerumDLL);
                Free_Serum();
                serum_Dispose();
                return false;
            }
        }
        if (width64 > 0)
        {
            MyNewFrame.frame64 = (UINT16*)malloc(2 * 64 * width64);
            MyNewFrame.rotations64 = (UINT16*)malloc(2 * MAX_COLOR_ROTATIONN * MAX_LENGTH_COLOR_ROTATION);
            MyNewFrame.rotationsinframe64 = (UINT16*)malloc(2 * 2 * 64 * width64);
            MyNewFrame.width64 = &width64;
            if (!MyNewFrame.frame64 || !MyNewFrame.rotations64 || !MyNewFrame.rotationsinframe64)
            {
                cprintf(false, "Can't get memory for the 32P elements of the DLL with new format");
                FreeLibrary(hSerumDLL);
                Free_Serum();
                serum_Dispose();
                return false;
            }
        }
    }
    return true;
}

bool InitLibSerum(char* vpdir, char* fname)
{
    if (!Load_Serum_DLL()) return false;
    char tbuf[MAX_PATH];
    strcpy_s(tbuf, MAX_PATH, vpdir);
    if (tbuf[strlen(tbuf) - 1] != '\\' && tbuf[strlen(tbuf) - 1] != '/') strcat_s(tbuf, MAX_PATH, "\\");
    strcat_s(tbuf, MAX_PATH, "altcolor\\");
    if (!serum_Load(tbuf, fname, &noColors, &ntriggers, FLAG_REQUEST_32P_FRAMES | FLAG_REQUEST_64P_FRAMES, &width32, &width64, &isNewFormat))
    {
        cprintf(false, "Serum DLL can't load the file, check the directory");
        FreeLibrary(hSerumDLL);
        return false;
    }
    if (!Allocate_Serum()) return false;
    return true;
}
///


/// <summary>
/// colorize a frame (in return, buffers are available if non NULL)
/// </summary>
/// <param name="vpframe">inbound frame</param>
/// <param name="oldframe">colorized frame if old format</param>
/// <param name="newframe32">colorized frame in 32P if new format</param>
/// <param name="newframe64">colorized frame in 32P if new format</param>
/// <returns>return true if there is an active color rotation, false if not</returns>
bool ColorizeAFrame(UINT8* vpframe, UINT8** oldframe, UINT16** newframe32, UINT16** newframe64, UINT* frameID, bool* is32fr, bool* is64fr)
{
    if (isNewFormat == 0)
    {
        MyOldFrame.frameID = frameID;
        serum_Colorize(vpframe, &MyOldFrame, NULL);
        *oldframe = MyOldFrame.frame;
        *newframe32 = NULL;
        *newframe64 = NULL;
        for (int ti = 0; ti < MAX_COLOR_ROTATIONS; ti++)
        {
            if (MyOldFrame.rotations[3 * ti] != 255) return true;
        }
    }
    else
    {
        MyNewFrame.frameID = frameID;
        serum_Colorize(vpframe, NULL, &MyNewFrame);
        *newframe32 = MyNewFrame.frame32;
        *newframe64 = MyNewFrame.frame64;
        *oldframe = NULL;
        if (*MyNewFrame.width32 > 0) *is32fr = true; else *is32fr = false;
        if (*MyNewFrame.width64 > 0) *is64fr = true; else *is64fr = false;
        for (int ti = 0; ti < MAX_COLOR_ROTATIONN; ti++)
        {
            if (MyNewFrame.rotations32[MAX_LENGTH_COLOR_ROTATION * ti] != 0) return true;
            if (MyNewFrame.rotations64[MAX_LENGTH_COLOR_ROTATION * ti] != 0) return true;
        }
    }
    return false;
}

bool ColorRotateAFrame(UINT8** oldframe, UINT16** newframe32, UINT8** modelt32, UINT16** newframe64, UINT8** modelt64)
{
    bool isrot;
    if (isNewFormat)
    {
        isrot = serum_ApplyRotationsN(&MyNewFrame, ModifiedElements32, ModifiedElements64); // if you don't need them replace ModifiedElementsXX by NULL
        *newframe32 = MyNewFrame.frame32;
        *newframe64 = MyNewFrame.frame64;
        if (modelt32) *modelt32 = ModifiedElements32;
        if (modelt64) *modelt64 = ModifiedElements64;
        *oldframe = NULL;
    }
    else
    {
        isrot = serum_ApplyRotations(&MyOldFrame);
        *oldframe = MyOldFrame.frame;
        *newframe32 = NULL;
        *newframe64 = NULL;
    }
    return isrot;
}

void StopLibSerum(void)
{
    Free_Serum();
    serum_Dispose();
    FreeLibrary(hSerumDLL);
}

