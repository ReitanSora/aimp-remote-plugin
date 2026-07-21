#include "pch.h"
#include "TrackRoutes.h"
#include "../core/Plugin.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// =============================================================================
// Helper Functions
// =============================================================================

void __stdcall OnAlbumArtReceive(IAIMPImage *Image, IAIMPImageContainer *Container, void *UserData)
{
    auto *imageBuffer = static_cast<std::vector<unsigned char> *>(UserData);
    if (Container)
    {
        DWORD size = Container->GetDataSize();
        byte *dataPtr = Container->GetData();
        if (dataPtr && size > 0)
        {
            imageBuffer->assign(dataPtr, dataPtr + size);
        }
    }
}

std::string get_mime_type(const std::vector<unsigned char> &data)
{
    if (data.size() < 4)
        return "image/jpeg";

    // JPEG: FF D8 FF
    if (data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF)
    {
        return "image/jpeg";
    }
    // PNG: 89 50 4E 47
    if (data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47)
    {
        return "image/png";
    }
    // GIF: 47 49 46 38
    if (data[0] == 0x47 && data[1] == 0x49 && data[2] == 0x46 && data[3] == 0x38)
    {
        return "image/gif";
    }
    // WebP: RIFF .... WEBP
    if (data.size() > 12 && data[0] == 'R' && data[1] == 'I' && data[2] == 'F' && data[3] == 'F' &&
        data[8] == 'W' && data[9] == 'E' && data[10] == 'B' && data[11] == 'P')
    {
        return "image/webp";
    }

    return "image/jpeg"; // Fallback
}

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