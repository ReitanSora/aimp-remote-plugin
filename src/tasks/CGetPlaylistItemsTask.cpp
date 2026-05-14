#include "pch.h"
#include "CGetPlaylistItemsTask.h"

CGetPlaylistItemsTask::CGetPlaylistItemsTask(MyPlugin *plugin, const std::string &id)
    : _plugin(plugin), _playlistId(id)
{
}

HRESULT WINAPI CGetPlaylistItemsTask::QueryInterface(REFIID riid, void **ppvObject)
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

ULONG WINAPI CGetPlaylistItemsTask::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI CGetPlaylistItemsTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CGetPlaylistItemsTask::Execute(IAIMPTaskOwner *Owner)
{
    if (!_plugin)
        return;

    IAIMPServicePlaylistManager *manager = _plugin->GetPlaylistService();
    IAIMPString *idPlaylist = nullptr;

    if (FAILED(_plugin->CreateAIMPString(_playlistId, &idPlaylist)))
    {
        return;
    }

    IAIMPPlaylist *playlist = nullptr;
    if (FAILED(manager->GetLoadedPlaylistByID(idPlaylist, &playlist)))
    {
        idPlaylist->Release();
        return;
    }

    int count = playlist->GetItemCount();

    for (int i = 0; i < count; i++)
    {
        if (Owner && Owner->IsCanceled())
            break;

        IAIMPPlaylistItem *item = nullptr;
        if (FAILED(playlist->GetItem(i, IID_IAIMPPlaylistItem, (void **)&item)))
        {
            continue;
        }

        IAIMPFileInfo *fileInfo = nullptr;
        if (SUCCEEDED(item->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_FILEINFO, IID_IAIMPFileInfo, (void **)&fileInfo)))
        {
            SongData song;
            song.index = std::to_string(i);
            song.title = _plugin->GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_TITLE, "Unknown Title");
            song.artist = _plugin->GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_ARTIST, "Unknown Artist");
            song.album = _plugin->GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_ALBUM, "Unknown Album");

            int bitrate = 0;
            int sampleRate = 0;
            double duration = 0.0;

            fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_BITRATE, &bitrate);
            fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_SAMPLERATE, &sampleRate);
            fileInfo->GetValueAsFloat(AIMP_FILEINFO_PROPID_DURATION, &duration);

            song.bitrate = bitrate;
            song.sampleRate = sampleRate;
            song.duration = duration;

            _results.push_back(song);
            fileInfo->Release();
        }

        item->Release();
    }

    playlist->Release();
    idPlaylist->Release();
}