#include "pch.h"
#include "CGetTrackLyricsTask.h"

CGetTrackLyricsTask::CGetTrackLyricsTask(MyPlugin* plugin, const std::string& playlistId, int songIndex)
    : _plugin(plugin), _playlistId(playlistId), _songIndex(songIndex) {}

HRESULT WINAPI CGetTrackLyricsTask::QueryInterface(REFIID riid, void** ppvObject)
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

ULONG WINAPI CGetTrackLyricsTask::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI CGetTrackLyricsTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CGetTrackLyricsTask::Execute(IAIMPTaskOwner* Owner)
{
    if (!_plugin || (Owner && Owner->IsCanceled())) return;
    
    IAIMPServicePlaylistManager* manager = _plugin->GetPlaylistService();
    if (!manager) return;

    IAIMPString* idPlaylist = nullptr;
    if (FAILED(_plugin->CreateAIMPString(_playlistId, &idPlaylist))) return;

    IAIMPPlaylist* playlist = nullptr;
    if (FAILED(manager->GetLoadedPlaylistByID(idPlaylist, &playlist)))
    {
        idPlaylist->Release();
        return;
    }

    IAIMPPlaylistItem* item = nullptr;
    if (FAILED(playlist->GetItem(_songIndex, IID_IAIMPPlaylistItem, (void**)&item)))
    {
        playlist->Release();
        idPlaylist->Release();
        return;
    }

    IAIMPFileInfo* fileInfo = nullptr;
    if (SUCCEEDED(item->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_FILEINFO, IID_IAIMPFileInfo, (void**)&fileInfo)))
    {
        IAIMPString* lyricsString = nullptr;
        if (SUCCEEDED(fileInfo->GetValueAsObject(AIMP_FILEINFO_PROPID_LYRICS, IID_IAIMPString, (void**)&lyricsString)))
        {
            TChar* rawText = lyricsString->GetData();
            int length = lyricsString->GetLength();

            if (rawText && length > 0) {
                int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, rawText, length, NULL, 0, NULL, NULL);
                if (sizeNeeded > 0)
                {
                    m_outLyricsUtf8.resize(sizeNeeded);
                    WideCharToMultiByte(CP_UTF8, 0, rawText, length, &m_outLyricsUtf8[0], sizeNeeded, NULL, NULL);
                }
            }

            lyricsString->Release();
        }
        fileInfo->Release();
    }

    item->Release();
    playlist->Release();
    idPlaylist->Release();
}