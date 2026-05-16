////////////////////////////////////////////////////////////////////////////////
//
//  Project:   AIMP
//             Remote Control Plugin
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

extern "C" __declspec(dllexport) TChar *WINAPI InfoGet(int Index)
{
    switch (Index)
    {
    case AIMP_PLUGIN_INFO_NAME:
        return (TChar *)L"AIMP Remote";
    case AIMP_PLUGIN_INFO_AUTHOR:
        return (TChar *)L"Stiven Pilca";
    case AIMP_PLUGIN_INFO_SHORT_DESCRIPTION:
        return (TChar *)L"Simple server with endpoints to control AIMP remotely";
    }
    return nullptr;
}

extern "C" __declspec(dllexport) LongWord WINAPI InfoGetCategories()
{
    return AIMP_PLUGIN_CATEGORY_ADDONS;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) { return TRUE; }