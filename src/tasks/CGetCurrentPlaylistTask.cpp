#include "pch.h"
#include "CGetCurrentPlaylistTask.h"

CGetCurrentPlaylistTask::CGetCurrentPlaylistTask(MyPlugin *plugin) : _plugin(plugin) {}

HRESULT WINAPI CGetCurrentPlaylistTask::QueryInterface(REFIID riid, void **ppvObject)
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

ULONG WINAPI CGetCurrentPlaylistTask::AddRef() { return InterlockedIncrement(&_refCount); }

ULONG WINAPI CGetCurrentPlaylistTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CGetCurrentPlaylistTask::Execute(IAIMPTaskOwner *Owner)
{
    if (!_plugin) return;

    IAIMPServicePlayer *player = nullptr;
    if (FAILED(_plugin->GetCore()->QueryInterface(IID_IAIMPServicePlayer, (void **)&player))) return;

    // Get the currently playing IAIMPPlaylistItem
    IAIMPPlaylistItem *playlistItem = nullptr;
    if (FAILED(player->GetPlaylistItem(&playlistItem)))
    {
        player->Release();
        return;
    }
    player->Release();

    // From the item, get the IAIMPPlaylist it belongs to
    IAIMPPlaylist *playlist = nullptr;
    if (FAILED(playlistItem->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_PLAYLIST, IID_IAIMPPlaylist, (void **)&playlist)))
    {
        playlistItem->Release();
        return;
    }
    playlistItem->Release();

    // Read id and name from the playlist's IAIMPPropertyList
    IAIMPPropertyList *propList = nullptr;
    if (SUCCEEDED(playlist->QueryInterface(IID_IAIMPPropertyList, (void **)&propList)))
    {
        _result.id = _plugin->GetPropertyText(propList, AIMP_PLAYLIST_PROPID_ID, "");
        _result.name = _plugin->GetPropertyText(propList, AIMP_PLAYLIST_PROPID_NAME, "Unknown");
        _result.itemCount = playlist->GetItemCount();
        _result.found = true;
        propList->Release();
    }

    playlist->Release();
}