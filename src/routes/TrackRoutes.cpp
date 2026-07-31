#include "pch.h"
#include "TrackRoutes.h"
#include "../core/Plugin.h"
#include <nlohmann/json.hpp>
#include "../helpers/AlbumArtHelper.h"

using json = nlohmann::json;

// =============================================================================
// Route Handlers
// =============================================================================

static void HandleGetTrackInfo(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    IAIMPFileInfo *fileInfo = nullptr;

    if (FAILED(plugin->GetPlayerService()->GetInfo(&fileInfo)))
    {
        json response = {
            {"success", false},
            {"error", "No track currently playing"}};

        res.status = 404;
        res.set_content(response.dump(), "application/json; charset=utf-8");
        return;
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

    int playCount = 0;
    int bitrate = 0;
    int sampleRate = 0;
    int rating = 0;

    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_BITRATE, &bitrate);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_SAMPLERATE, &sampleRate);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_ML_MARK, &rating);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_ML_PLAYCOUNT, &playCount);

    json response = {
        {"title", title},
        {"artist", artist},
        {"album", album},
        {"format", extension},
        {"genre", genre},
        {"play_count", playCount},
        {"bitrate", bitrate},
        {"sample_rate", sampleRate},
        {"rating", rating}};

    fileInfo->Release();

    res.status = 200;
    res.set_content(response.dump(), "application/json; charset=utf-8");
}

static void HandleGetTrackCover(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    IAIMPFileInfo *fileInfo = nullptr;

    if (FAILED(plugin->GetPlayerService()->GetInfo(&fileInfo)))
    {
        res.status = 404;
        res.set_content(json{
                            {"success", false},
                            {"error", "No track currently playing"}}
                            .dump(),
                        "application/json; charset=utf-8");
        return;
    }

    IAIMPString *fileURI = nullptr;

    if (FAILED(fileInfo->GetValueAsObject(AIMP_FILEINFO_PROPID_FILENAME, IID_IAIMPString, (void **)&fileURI)))
    {
        fileInfo->Release();
        res.status = 500;
        res.set_content(json{
                            {"success", false},
                            {"error", "Failed to get track file URI"}}
                            .dump(),
                        "application/json; charset=utf-8");
        return;
    }

    std::vector<unsigned char> imageBytes;
    TTaskHandle taskID = 0;

    HRESULT hr = plugin->GetAlbumArtService()->Get(fileURI, nullptr, nullptr, AIMP_SERVICE_ALBUMART_FLAGS_ORIGINAL, OnAlbumArtReceive, &imageBytes, &taskID);

    int attemps = 0;
    while (imageBytes.empty() && attemps < 50)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        attemps++;
    }

    fileURI->Release();
    fileInfo->Release();

    if (!imageBytes.empty())
    {
        std::string mimeType = get_mime_type(imageBytes);

        res.set_content(reinterpret_cast<const char *>(imageBytes.data()), imageBytes.size(), mimeType);
    }
    else
    {
        json response = {
            {"success", false},
            {"error", "Album art not found or retrieval timeout"}};

        res.status = 404;
        res.set_content(response.dump(), "application/json; charset=utf-8");
        return;
    }
}

// =============================================================================
// Route Registration
// =============================================================================

void RegisterTrackRoutes(MyPlugin* plugin)
{
    auto &svr = plugin->GetHttpServer();

    // GET endpoints
    svr.Get("/track/info", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetTrackInfo(plugin, req, res); });

    svr.Get("/track/cover", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetTrackCover(plugin, req, res); });
}