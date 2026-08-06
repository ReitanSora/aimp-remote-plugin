////////////////////////////////////////////////////////////////////////////////
//
//  Project:   Fluke
//             AIMP Remote Control Plugin
//
//  Target:    v5.40 build 2709
//
//  Purpose:   Remote Plugin
//
//  Author:    ReitanSora
//             © 2026
//

#include "pch.h"
#include "core/Plugin.h"

extern "C" __declspec(dllexport) HRESULT WINAPI AIMPPluginGetHeader(IAIMPPlugin **Header)
{
    *Header = new MyPlugin();
    return S_OK;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) { return TRUE; }