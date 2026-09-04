#include "pch.h"
#include "CGetTrackInfoTask.h"

CGetTrackInfoTask::CGetTrackInfoTask(MyPlugin* plugin) : _plugin(plugin) {}

HRESULT WINAPI CGetTrackInfoTask::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAIMPTask))
    {
        *ppvObject = this;
        AddRef();
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG WINAPI CGetTrackInfoTask::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI CGetTrackInfoTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CGetTrackInfoTask::Execute(IAIMPTaskOwner* Owner)
{
    if (_plugin)
    {
        _result = GetTrackInfo(_plugin);
    }
}