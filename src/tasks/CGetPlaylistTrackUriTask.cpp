#include "pch.h"
#include "CGetPlaylistTrackUriTask.h"
#include "../helpers/AlbumArtHelper.h"

CGetPlaylistTrackUriTask::CGetPlaylistTrackUriTask(MyPlugin* plugin, const std::string& playlistId)
    : _plugin(plugin), _playlistId(playlistId) {}

HRESULT WINAPI CGetPlaylistTrackUriTask::QueryInterface(REFIID riid, void** ppvObject)
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

ULONG WINAPI CGetPlaylistTrackUriTask::AddRef() { return InterlockedIncrement(&_refCount); }

ULONG WINAPI CGetPlaylistTrackUriTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CGetPlaylistTrackUriTask::Execute(IAIMPTaskOwner* Owner)
{
    if (!_plugin) return;

    IAIMPServicePlaylistManager* manager = _plugin->GetPlaylistService();
    IAIMPString* idPlaylist = nullptr;

    if (FAILED(_plugin->CreateAIMPString(_playlistId, &idPlaylist))) return;

    IAIMPPlaylist* playlist = nullptr;
    if (FAILED(manager->GetLoadedPlaylistByID(idPlaylist, &playlist)))
    {
        idPlaylist->Release();
        return;
    }

    int count = playlist->GetItemCount();
    int maxItems = (count < 10) ? count : 10;

    for (int i = 0; i < maxItems; i++)
    {
        if (Owner && Owner->IsCanceled()) break;

        IAIMPPlaylistItem* item = nullptr;
        if (FAILED(playlist->GetItem(i, IID_IAIMPPlaylistItem, (void**)&item))) continue;

        IAIMPFileInfo* fileInfo = nullptr;
        if (SUCCEEDED(item->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_FILEINFO, IID_IAIMPFileInfo, (void**)&fileInfo)))
        {
            std::string uri = _plugin->GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_FILENAME, "");
            if (!uri.empty())
            {
                _fileUris.push_back(uri);
            }
            fileInfo->Release();
        }
        item->Release();
    }

    playlist->Release();
    idPlaylist->Release();
}