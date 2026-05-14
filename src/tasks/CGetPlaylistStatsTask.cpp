#include "pch.h"
#include "CGetPlaylistStatsTask.h"

CGetPlaylistStatsTask::CGetPlaylistStatsTask(MyPlugin *plugin, const std::string &id)
    : _plugin(plugin), _playlistId(id)
{
}

HRESULT WINAPI CGetPlaylistStatsTask::QueryInterface(REFIID riid, void **ppvObject)
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
ULONG WINAPI CGetPlaylistStatsTask::AddRef() { return InterlockedIncrement(&_refCount); }
ULONG WINAPI CGetPlaylistStatsTask::Release()
{
    ULONG c = InterlockedDecrement(&_refCount);
    if (c == 0)
        delete this;
    return c;
}

void WINAPI CGetPlaylistStatsTask::Execute(IAIMPTaskOwner *Owner)
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

    int count = playlist->GetItemCount();

    std::set<std::string> genreSet;
    std::set<std::string> artistSet;
    std::set<std::string> albumSet;

    long long totalBitrate = 0;
    int bitrateCount = 0;
    double totalRating = 0.0;

    for (int i = 0; i < count; i++)
    {
        if (Owner && Owner->IsCanceled())
            break;

        IAIMPPlaylistItem *item = nullptr;
        if (FAILED(playlist->GetItem(i, IID_IAIMPPlaylistItem, (void **)&item)))
            continue;

        IAIMPFileInfo *fi = nullptr;
        if (SUCCEEDED(item->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_FILEINFO,
                                             IID_IAIMPFileInfo, (void **)&fi)))
        {
            // Unique sets
            std::string genre = _plugin->GetPropertyText(fi, AIMP_FILEINFO_PROPID_GENRE, "");
            std::string artist = _plugin->GetPropertyText(fi, AIMP_FILEINFO_PROPID_ARTIST, "");
            std::string album = _plugin->GetPropertyText(fi, AIMP_FILEINFO_PROPID_ALBUM, "");

            if (!genre.empty())
                genreSet.insert(genre);
            if (!artist.empty())
                artistSet.insert(artist);
            if (!album.empty())
                albumSet.insert(album);

            // Bitrate
            int bitrate = 0;
            if (SUCCEEDED(fi->GetValueAsInt32(AIMP_FILEINFO_PROPID_BITRATE, &bitrate)) && bitrate > 0)
            {
                totalBitrate += bitrate;
                bitrateCount++;
            }

            // Rating (ML rating, float 0..5)
            double rating = 0.0;
            fi->GetValueAsFloat(AIMP_FILEINFO_PROPID_ML_RATING, &rating);
            if (rating > 0.0)
            {
                totalRating += rating;
                _result.tracksWithRating++;
            }

            // Play count
            int playCount = 0;
            fi->GetValueAsInt32(AIMP_FILEINFO_PROPID_ML_PLAYCOUNT, &playCount);
            _result.totalPlayCount += playCount;
            if (playCount == 0)
                _result.tracksNeverPlayed++;

            INT64 fileSize = 0;
            fi->GetValueAsInt64(AIMP_FILEINFO_PROPID_FILESIZE, &fileSize);
            _result.totalSizeBytes += fileSize;

            fi->Release();
        }

        item->Release();
    }

    // Populate result
    _result.genres.assign(genreSet.begin(), genreSet.end());
    _result.artists.assign(artistSet.begin(), artistSet.end());
    _result.artistCount = static_cast<int>(artistSet.size());
    _result.albumCount = static_cast<int>(albumSet.size());
    _result.avgBitrate = bitrateCount > 0
                             ? static_cast<double>(totalBitrate) / bitrateCount
                             : 0.0;
    _result.avgRating = _result.tracksWithRating > 0
                            ? totalRating / _result.tracksWithRating
                            : 0.0;
    _result.found = true;

    playlist->Release();
}