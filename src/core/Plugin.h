#pragma once

#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <string>

// AIMP SDK headers
#include "../../sdk/aimp/5.40/apiPlugin.h"
#include "../../sdk/aimp/5.40/apiMessages.h"
#include "../../sdk/aimp/5.40/apiRemote.h"
#include "../../sdk/aimp/5.40/apiAlbumArt.h"
#include "../../sdk/aimp/5.40/apiOptions.h"
#include "../../sdk/aimp/5.40/apiGUI.h"

#include <nlohmann/json_fwd.hpp>

namespace httplib 
{ 
    class Server; 
}

namespace ix 
{ 
    class WebSocket; 
    class WebSocketServer; 
}

static const GUID IID_IAIMPPlugin =
    {0x41494D50, 0x506C, 0x7567, {0x69, 0x6E, 0x48, 0x64, 0x72, 0x49, 0x44, 0x00}};

/**
 * @class MyPlugin
 * @brief Main plugin class for AIMP Remote Control
 *
 * Implements both IAIMPPlugin (lifecycle) and IAIMPMessageHook (real-time events).
 * Provides REST API via httplib and WebSocket events via ixwebsocket.
 */
class MyPlugin : public IAIMPPlugin, public IAIMPMessageHook, public IAIMPOptionsDialogFrame
{
private:
    // =========================================================================
    // COM Reference Counting
    // =========================================================================
    ULONG _refCount = 1;

    // =========================================================================
    // HTTP REST Server (port 3553)
    // =========================================================================
    std::unique_ptr<httplib::Server> _httpServer;
    std::unique_ptr<std::thread> _httpThread;

    // =========================================================================
    // WebSocket Server (port 3554)
    // =========================================================================
    std::unique_ptr<ix::WebSocketServer> _wsServer;
    std::mutex _wsMutex;
    std::vector<std::shared_ptr<ix::WebSocket>> _wsClients;

    // =========================================================================
    // AIMP Services
    // =========================================================================
    IAIMPCore *_core = nullptr;
    IAIMPServicePlayer *_playerService = nullptr;
    IAIMPServiceAlbumArt *_albumArtService = nullptr;
    IAIMPServicePlaylistManager *_playlistService = nullptr;
    IAIMPServiceThreads *_threadService = nullptr;
    IAIMPServiceOptionsDialog* _optionsService = nullptr;
    IAIMPServiceUI* _uiService = nullptr;
    IAIMPUIForm* _uiForm = nullptr;
    IAIMPUILabel* _ipLabel = nullptr;

    // =========================================================================
    // Private Helper Methods
    // =========================================================================

    /**
     * @brief Builds JSON object with current track metadata
     * @return JSON object with track info (title, artist, album, etc.)
     * @note Must be called from AIMP's main thread
     */
    nlohmann::json BuildTrackInfoJson();

    /**
     * @brief Converts wide string (UTF-16) to UTF-8
     * @param wstr Wide string to convert
     * @return UTF-8 encoded string
     */
    std::string WideToUTF8(const WCHAR *wstr);

    /**
     * @brief Converts UTF-8 string to wide string (UTF-16)
     * @param str UTF-8 string to convert
     * @return Wide string
     */
    std::wstring Utf8ToWide(const std::string &str);

public:
    httplib::Server& GetHttpServer() { return *_httpServer; }
    // =========================================================================
    // Public Accessors (for Tasks)
    // =========================================================================

    /**
     * @brief Gets the AIMP core service
     * @return Pointer to IAIMPCore
     */
    IAIMPCore *GetCore() { return _core; };

    /**
     * @brief Gets the player service
     * @return Pointer to IAIMPServicePlayer
     */
    IAIMPServicePlayer *GetPlayerService() { return _playerService; };

    /**
     * @brief Gets the album art service
     * @return Pointer to IAIMPServiceAlbumArt
     */
    IAIMPServiceAlbumArt *GetAlbumArtService() { return _albumArtService; };

    /**
     * @brief Gets the playlist manager service
     * @return Pointer to IAIMPServicePlaylistManager
     */
    IAIMPServicePlaylistManager *GetPlaylistService() { return _playlistService; };

    /**
     * @brief Gets thread service for async operations
     * @return Pointer to IAIMPServiceThreads
     */
    IAIMPServiceThreads *GetThreadService() { return _threadService; };

    /**
     * @brief Broadcasts JSON message to all connected WebSocket clients
     * @param msg JSON object to broadcast
     * @note Thread-safe; automatically removes dead connections
     */
    void BroadcastWS(const nlohmann::json &msg);

    // =========================================================================
    // Property Extraction Helpers
    // =========================================================================

    /**
     * @brief Extracts string property from playlist property list
     * @param info Property list object
     * @param propID Property ID to extract
     * @param defaultValue Default value if property is missing
     * @return Property value as UTF-8 string
     */
    std::string GetPropertyTextPlaylist(IAIMPPropertyList *info, int propID, const std::string &defaultValue = "Unknown");

    /**
     * @brief Extracts string property from file info
     * @param info File info object
     * @param propID Property ID to extract
     * @param defaultValue Default value if property is missing
     * @return Property value as UTF-8 string
     */
    std::string GetPropertyText(IAIMPFileInfo *info, int propID, const std::string &defaultValue = "Unknown");

    HRESULT CreateAIMPString(const std::string& utf8Text, IAIMPString** ppAIMPString);

    HRESULT WINAPI GetName(IAIMPString** S) override;
    HWND WINAPI CreateFrame(HWND ParentWnd) override;
    void WINAPI DestroyFrame() override;
    void WINAPI Notification(int ID) override;

    // =========================================================================
    // IAIMPPlugin Interface
    // =========================================================================

    HRESULT WINAPI Initialize(IAIMPCore *Core) override;
    HRESULT WINAPI Finalize() override;
    void WINAPI SystemNotification(int NotifyID, IUnknown *Data) override;
    TChar* WINAPI InfoGet(int Index) override;
    LongWord WINAPI InfoGetCategories() override;

    // =========================================================================
    // IAIMPMessageHook Interface
    // =========================================================================

    /**
     * @brief Receives AIMP core messages (track changes, player state, etc.)
     * @param AMessage Message ID (AIMP_MSG_EVENT_*)
     * @param AParam1 First parameter (context-dependent)
     * @param AParam2 Second parameter (context-dependent)
     * @param AResult Result code (can be modified)
     */
    virtual void WINAPI CoreMessage(LongWord AMessage, int AParam1, void *AParam2, HRESULT *AResult) override;

    // =========================================================================
    // IUnknown Interface
    // =========================================================================

    HRESULT WINAPI QueryInterface(REFIID riid, void **ppvObject) override;
    ULONG WINAPI AddRef() override;
    ULONG WINAPI Release() override;
};

/**
 * @brief Factory function to create plugin instance
 * @param Core AIMP core service
 * @param Unknown Output pointer for plugin instance
 * @return S_OK on success
 */
extern "C" {
    __declspec(dllexport) HRESULT WINAPI AIMPPluginGetHeader(IAIMPPlugin** Unknown);
    __declspec(dllexport) TChar* WINAPI InfoGet(int Index);
    __declspec(dllexport) LongWord WINAPI InfoGetCategories();
}