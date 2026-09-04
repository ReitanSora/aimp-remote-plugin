#include "pch.h"
#include "TrackInfoHelper.h"

json GetTrackInfo(MyPlugin* plugin)
{

    IAIMPFileInfo* fileInfo = nullptr;

    if (FAILED(plugin->GetPlayerService()->GetInfo(&fileInfo)))
    {
        json response = {
            {"success", false},
            {"error", "No track currently playing"} };

        return response;
    }

    std::string title = plugin->GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_TITLE, "Untitled");
    std::string artist = plugin->GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_ARTIST, "Unknown Artist");
    std::string album = plugin->GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_ALBUM, "Unknown Album");
    std::string genre = plugin->GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_GENRE, "Unknown Genre");
    std::string filename = plugin->GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_FILENAME, "Unknown Format");
    std::string extension = "unknown";

    size_t dotPos = filename.find_last_of(".");
    if (dotPos != std::string::npos && dotPos + 1 < filename.length()) {
        extension = filename.substr(dotPos + 1);

        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    }
    
    double duration = 0.0;
    int bitrate = 0;
    int playCount = 0;
    int sampleRate = 0;
    int rating = 0;

    fileInfo->GetValueAsFloat(AIMP_FILEINFO_PROPID_DURATION, &duration);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_BITRATE, &bitrate);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_SAMPLERATE, &sampleRate);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_ML_MARK, &rating);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_ML_PLAYCOUNT, &playCount);

    std::string playlistId = "";
    int itemId = 0;

    IAIMPPlaylistItem* plItem = nullptr;
    if (SUCCEEDED(plugin->GetPlayerService()->GetPlaylistItem(&plItem)))
    {
        
        IAIMPPlaylist* playlist = nullptr;
        if (SUCCEEDED(plItem->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_PLAYLIST,
            IID_IAIMPPlaylist, (void**)&playlist)))
        {
            IAIMPPropertyList* propList = nullptr;
            if (SUCCEEDED(playlist->QueryInterface(IID_IAIMPPropertyList, (void**)&propList)))
            {
                playlistId = plugin->GetPropertyText(propList, AIMP_PLAYLIST_PROPID_ID, "");
                propList->Release();
            }
            playlist->Release();
        }
        int index = 0;
        if (SUCCEEDED(plItem->GetValueAsInt32(AIMP_PLAYLISTITEM_PROPID_INDEX, &index)))
        {
			itemId = index;
        }
        plItem->Release();
    }

    json response = {
        {"album", album},
        {"artist", artist},
        {"bitrate", bitrate},
        {"duration", duration},
        {"format", extension},
        {"genre", genre},
        {"index", itemId},
        {"play_count", playCount},
        {"playlist_id", playlistId},
        {"rating", rating},
        {"sample_rate", sampleRate},
        {"success", true},
        {"title", title },
    };

    fileInfo->Release();

    return response;
}