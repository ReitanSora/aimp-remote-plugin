#include "pch.h"
#include "CGetPlaylistInfoTask.h"

CGetPlaylistInfoTask::CGetPlaylistInfoTask(MyPlugin *plugin, const std::string &id)
    : _plugin(plugin), _playlistId(id)
{
}

HRESULT WINAPI CGetPlaylistInfoTask::QueryInterface(REFIID riid, void **ppvObject)
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

ULONG WINAPI CGetPlaylistInfoTask::AddRef() { return InterlockedIncrement(&_refCount); }

ULONG WINAPI CGetPlaylistInfoTask::Release()
{
    ULONG c = InterlockedDecrement(&_refCount);
    if (c == 0)
        delete this;
    return c;
}

void WINAPI CGetPlaylistInfoTask::Execute(IAIMPTaskOwner *Owner)
{
    if (!_plugin)
        return;

    IAIMPString *idStr = nullptr;
    if (FAILED(_plugin->CreateAIMPString(_playlistId, &idStr)))
        return;

    IAIMPPlaylist *playlist = nullptr;
    if (FAILED(_plugin->GetPlaylistService()->GetLoadedPlaylistByID(idStr, &playlist)))
    {
        idStr->Release();
        return;
    }
    idStr->Release();

    IAIMPPropertyList *props = nullptr;
    if (SUCCEEDED(playlist->QueryInterface(IID_IAIMPPropertyList, (void **)&props)))
    {

        _result.id = _plugin->GetPropertyTextPlaylist(props, AIMP_PLAYLIST_PROPID_ID, "");
        _result.name = _plugin->GetPropertyTextPlaylist(props, AIMP_PLAYLIST_PROPID_NAME, "Unknown");

        // Total duration in seconds (stored as double by AIMP)
        double dur = 0.0;
        props->GetValueAsFloat(AIMP_PLAYLIST_PROPID_DURATION, &dur);
        _result.duration = dur;

        // Index of the currently playing item in this playlist (-1 = nothing playing here)
        int playingIdx = -1;
        props->GetValueAsInt32(AIMP_PLAYLIST_PROPID_PLAYINGINDEX, &playingIdx);
        _result.playingIndex = playingIdx;

        // Read-only flag
        int readOnly = 0;
        props->GetValueAsInt32(AIMP_PLAYLIST_PROPID_READONLY, &readOnly);
        _result.isReadOnly = (readOnly != 0);

        _result.itemCount = playlist->GetItemCount();
        _result.found = true;

        props->Release();
    }

    playlist->Release();
}