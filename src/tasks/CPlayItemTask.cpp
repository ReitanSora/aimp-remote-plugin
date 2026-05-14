#include "pch.h"

#include "CPlayItemTask.h"

CPlayItemTask::CPlayItemTask(MyPlugin *plugin, const std::string &id, int index)
    : _plugin(plugin), _playlistId(id), _index(index)
{
}

HRESULT WINAPI CPlayItemTask::QueryInterface(REFIID riid, void **ppvObject)
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

ULONG WINAPI CPlayItemTask::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI CPlayItemTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CPlayItemTask::Execute(IAIMPTaskOwner *Owner)
{
    if (!_plugin)
        return;

    IAIMPServicePlayer *playerService = nullptr;
    IAIMPServicePlaylistManager *playlistManager = _plugin->GetPlaylistService();

    if (FAILED(_plugin->GetCore()->QueryInterface(IID_IAIMPServicePlayer, (void **)&playerService)))
        return;

    IAIMPString *idPlaylist = nullptr;
    if (SUCCEEDED(_plugin->CreateAIMPString(_playlistId, &idPlaylist)))
    {
        IAIMPPlaylist *playlist = nullptr;

        if (SUCCEEDED(playlistManager->GetLoadedPlaylistByID(idPlaylist, &playlist)))
        {

            if (_index >= 0 && _index < playlist->GetItemCount())
            {
                IAIMPPlaylistItem *item = nullptr;

                if (SUCCEEDED(playlist->GetItem(_index, IID_IAIMPPlaylistItem, (void **)&item)))
                {
                    playerService->Play2(item);
                    item->Release();
                }
            }
            playlist->Release();
        }
        idPlaylist->Release();
    }
    playerService->Release();
};