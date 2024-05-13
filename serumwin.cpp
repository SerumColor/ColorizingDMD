#include "serumwin.h"

HINSTANCE hSerumDLL;
Serum_LoadFunc Serum_Load;
Serum_DisposeFunc Serum_Dispose, Serum_DisableColorization, Serum_EnableColorization;
Serum_ColorizeFunc Serum_Colorize;
Serum_RotateFunc Serum_Rotate;
Serum_GetVersionFunc Serum_GetVersion, Serum_GetMinorVersion;

void Serum_ReleaseDLL(void)
{
    FreeLibrary(hSerumDLL);
}

bool Serum_LoadDLL(const char* File_SerumDLL)
{
    
    
    
    hSerumDLL = LoadLibraryA(File_SerumDLL);
    if (hSerumDLL == NULL)
    {
        
        return false;
    }
    Serum_Load = (Serum_LoadFunc)GetProcAddress(hSerumDLL, "Serum_Load");
    if (Serum_Load == NULL)
    {
        
        Serum_ReleaseDLL();
        return false;
    }
    Serum_Dispose = (Serum_DisposeFunc)GetProcAddress(hSerumDLL, "Serum_Dispose");
    if (Serum_Dispose == NULL)
    {
        
        Serum_ReleaseDLL();
        return false;
    }
    Serum_Colorize = (Serum_ColorizeFunc)GetProcAddress(hSerumDLL, "Serum_Colorize");
    if (Serum_Colorize == NULL)
    {
        
        Serum_ReleaseDLL();
        return false;
    }
    Serum_Rotate = (Serum_RotateFunc)GetProcAddress(hSerumDLL, "Serum_Rotate");
    if (Serum_Rotate == NULL)
    {
        
        Serum_ReleaseDLL();
        return false;
    }
    Serum_GetVersion = (Serum_GetVersionFunc)GetProcAddress(hSerumDLL, "Serum_GetVersion");
    if (Serum_GetVersion == NULL)
    {
        
        Serum_ReleaseDLL();
        return false;
    }
    Serum_GetMinorVersion = (Serum_GetVersionFunc)GetProcAddress(hSerumDLL, "Serum_GetMinorVersion");
    if (Serum_GetMinorVersion == NULL)
    {
        
        Serum_ReleaseDLL();
        return false;
    }
    Serum_DisableColorization = (Serum_DisposeFunc)GetProcAddress(hSerumDLL, "Serum_DisableColorization");
    if (Serum_DisableColorization == NULL)
    {
        
        Serum_ReleaseDLL();
        return false;
    }
    Serum_EnableColorization = (Serum_DisposeFunc)GetProcAddress(hSerumDLL, "Serum_EnableColorization");
    if (Serum_DisableColorization == NULL)
    {
        
        Serum_ReleaseDLL();
        return false;
    }
    return true;
}
