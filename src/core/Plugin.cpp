#include "pch.h"
#include "Plugin.h"

// =========================================================================
// QueryInterface
// Must expose IAIMPMessageHook so dispatcher->Hook(this) can QI for it
// internally and route CoreMessage() calls back to us.
// =========================================================================
HRESULT WINAPI MyPlugin::QueryInterface(REFIID riid, void **ppvObject)
{
    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAIMPPlugin))
    {
        *ppvObject = static_cast<IAIMPPlugin *>(this);
        AddRef();
        return S_OK;
    }

    if (IsEqualIID(riid, IID_IAIMPMessageHook))
    {
        *ppvObject = static_cast<IAIMPMessageHook *>(this);
        AddRef();
        return S_OK;
    }

    if (IsEqualIID(riid, IID_IAIMPOptionsDialogFrame))
    {
        *ppvObject = static_cast<IAIMPOptionsDialogFrame *>(this);
        AddRef();
        return S_OK;
    }

    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG WINAPI MyPlugin::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI MyPlugin::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

TChar* WINAPI MyPlugin::InfoGet(int Index)
{
    switch (Index)
    {
    case AIMP_PLUGIN_INFO_NAME:
        return (TChar*)L"Fluke: AIMP Remote Control";
    case AIMP_PLUGIN_INFO_AUTHOR:
        return (TChar*)L"Stiven Pilca";
    case AIMP_PLUGIN_INFO_SHORT_DESCRIPTION:
        return (TChar*)L"Remote control plugin for AIMP";
    }
    return nullptr;
}

LongWord WINAPI MyPlugin::InfoGetCategories()
{
    return AIMP_PLUGIN_CATEGORY_ADDONS;
}