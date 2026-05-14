#include "pch.h"
#include "Plugin.h"

// Task headers
#include "../tasks/CGetCurrentPlaylistTask.h"
#include "../tasks/CPlayItemTask.h"
#include "../tasks/CGetPlaylistItemsTask.h"
#include "../tasks/CGetPlaylistsTask.h"
#include "../tasks/CGetPlaylistStatsTask.h"
#include "../tasks/CGetPlaylistInfoTask.h"
#include "Config.h"
#include "../routes/PlayerRoutes.h"
#include "../routes/PlaylistRoutes.h"
#include "../routes/TrackRoutes.h"

// -------------------------------------------------------------------------
/// @brief BuildTrackInfoJson
///
/// Reads metadata for the currently playing track from IAIMP's player
/// service and returns it as a JSON object.
///
/// Notes:
/// - MUST be called from AIMP's main thread (for example, inside CoreMessage).
///   The IAIMP COM services used here are not thread-safe and can fail if
///   invoked from worker threads.
/// - If the player service is not available or there is no currently loaded
///   track, the function returns an empty JSON object.
///
/// Behavior & resource management:
/// - Queries string properties using `GetPropertyText` and playlist helpers,
///   and numeric properties using `GetValueAsFloat` / `GetValueAsInt32`.
/// - All COM objects acquired (`IAIMPFileInfo`, `IAIMPPlaylistItem`,
///   `IAIMPPlaylist`, `IAIMPPropertyList`, etc.) are properly `Release()`d
///   before the function returns to avoid leaks.
///
/// Returned JSON schema (keys are always present if function succeeds):
/// {
///   "title": string,        // track title or "Unknown"
///   "artist": string,       // track artist or "Unknown"
///   "album": string,        // album name or "Unknown"
///   "genre": string,        // genre or "Unknown"
///   "duration": number,     // duration in seconds (double)
///   "play_count": integer,  // play count (int)
///   "playlist_id": string,  // ID of the playlist containing the track (may be "")
///   "bitrate": integer,     // bitrate in kbps (int)
///   "sample_rate": integer, // sample rate in Hz (int)
///   "rating": integer       // user rating / ML mark (int)
/// }
///
/// Example:
/// {"title":"Track Name","artist":"Artist","album":"Album","duration":215.0, ...}
///
/// @return nlohmann::json JSON object with track metadata or an empty object on failure.
// -------------------------------------------------------------------------
json MyPlugin::BuildTrackInfoJson()
{
    if (!_playerService)
        return json::object();

    IAIMPFileInfo *fileInfo = nullptr;
    if (FAILED(_playerService->GetInfo(&fileInfo)))
        return json::object();

    std::string title = GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_TITLE, "Unknown");
    std::string artist = GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_ARTIST, "Unknown");
    std::string album = GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_ALBUM, "Unknown");
    std::string genre = GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_GENRE, "Unknown");

    double duration = 0.0;
    int bitrate = 0;
    int sampleRate = 0;
    int playCount = 0;
    int rating = 0;
    fileInfo->GetValueAsFloat(AIMP_FILEINFO_PROPID_DURATION, &duration);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_BITRATE, &bitrate);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_SAMPLERATE, &sampleRate);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_ML_MARK, &rating);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_ML_PLAYCOUNT, &playCount);
    fileInfo->Release();

    // Read the playlist the current track belongs to.
    // GetPlaylistItem() only succeeds while a track is loaded.
    std::string playlistId = "";

    IAIMPPlaylistItem *plItem = nullptr;
    if (SUCCEEDED(_playerService->GetPlaylistItem(&plItem)))
    {
        IAIMPPlaylist *playlist = nullptr;
        if (SUCCEEDED(plItem->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_PLAYLIST,
                                               IID_IAIMPPlaylist, (void **)&playlist)))
        {
            IAIMPPropertyList *propList = nullptr;
            if (SUCCEEDED(playlist->QueryInterface(IID_IAIMPPropertyList, (void **)&propList)))
            {
                playlistId = GetPropertyTextPlaylist(propList, AIMP_PLAYLIST_PROPID_ID, "");
                propList->Release();
            }
            playlist->Release();
        }
        plItem->Release();
    }

    return {
        {"title", title},
        {"artist", artist},
        {"album", album},
        {"genre", genre},
        {"duration", duration},
        {"play_count", playCount},
        {"playlist_id", playlistId},
        {"bitrate", bitrate},
        {"sample_rate", sampleRate},
        {"rating", rating}};
}

// -------------------------------------------------------------------------
// String helpers
// -------------------------------------------------------------------------

/**
 * @brief Convert a wide-character (UTF-16) string to a UTF-8 encoded std::string.
 *
 * This utility wraps the Windows API {@code WideCharToMultiByte} to convert
 * a NUL-terminated wide string (WCHAR const*) into a UTF-8 std::string.
 *
 * Notes:
 * - If {@code wstr} is null or points to an empty string, an empty string is
 *   returned.
 * - The returned std::string does NOT include a trailing NUL character.
 * - The function is safe for use on the AIMP main thread and worker threads,
 *   but callers must still respect COM thread-affinity rules for any COM
 *   objects they use alongside this function.
 *
 * @param wstr Pointer to a NUL-terminated wide-character string (UTF-16).
 * @return std::string UTF-8 encoded string (empty on null/empty input).
 */
std::string MyPlugin::WideToUTF8(const WCHAR *wstr)
{
    if (!wstr || wstr[0] == L'\0')
        return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, &strTo[0], size_needed, NULL, NULL);
    if (!strTo.empty() && strTo.back() == '\0')
        strTo.pop_back();
    return strTo;
}

/**
 * @brief Convert a UTF-8 encoded std::string to a wide-character (UTF-16) std::wstring.
 *
 * This utility wraps the Windows API `MultiByteToWideChar` to convert a UTF-8 byte
 * sequence held in a `std::string` into a `std::wstring`.
 *
 * Behavior and notes:
 * - If `str` is empty, an empty `std::wstring` is returned.
 * - The conversion uses `str.size()` (not a NUL-terminated c-string). This means
 *   embedded NUL bytes in `str` are preserved up to `str.size()`.
 * - The returned `std::wstring` represents the converted characters and does not
 *   include an extra terminating NUL in its logical length (though `c_str()` will
 *   provide a terminating NUL as usual).
 * - Uses `CP_UTF8` as the source code page.
 * - The function itself is thread-safe (no global state). Callers must still
 *   follow COM apartment/threading rules when using COM objects elsewhere.
 *
 * @param str UTF-8 encoded input string
 * @return std::wstring UTF-16 wide string converted from the input (empty on empty input)
 */
std::wstring MyPlugin::Utf8ToWide(const std::string &str)
{
    if (str.empty())
        return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

/**
 * @brief Extracts a UTF-8 string property from a playlist property list.
 *
 * Retrieves the property identified by `propID` from an `IAIMPPropertyList`
 * as an `IAIMPString`, converts it to a UTF-8 `std::string` and returns it.
 * If the property is not present, or the retrieved string is empty, the
 * provided `defaultValue` is returned.
 *
 * Notes:
 * - This function queries `info->GetValueAsObject` for `IID_IAIMPString`.
 *   If successful, the returned `IAIMPString` is released before returning
 *   to avoid leaking COM objects.
 * - The conversion from wide characters to UTF-8 uses `WideToUTF8`.
 * - Callers must respect COM apartment/threading rules for `info`; this
 *   helper itself does not change `info`'s reference count.
 *
 * @param info Pointer to an `IAIMPPropertyList` exposing the requested property.
 * @param propID Property identifier to retrieve (for example, `AIMP_PLAYLIST_PROPID_ID`).
 * @param defaultValue Value to return when the property is missing or empty.
 * @return std::string UTF-8 encoded property value or `defaultValue`.
 */
std::string MyPlugin::GetPropertyTextPlaylist(IAIMPPropertyList *info, int propID, const std::string &defaultValue)
{
    IAIMPString *aString = nullptr;
    std::string result = "";
    if (SUCCEEDED(info->GetValueAsObject(propID, IID_IAIMPString, (void **)&aString)))
    {
        result = WideToUTF8(aString->GetData());
        aString->Release();
    }
    return result.empty() ? defaultValue : result;
}

/**
 * @brief Retrieve a textual property from an IAIMPFileInfo as a UTF-8 std::string.
 *
 * This helper queries the provided `IAIMPFileInfo` for a property identified by
 * `propID` and requests the value as an `IAIMPString` COM object. If the call
 * succeeds the returned `IAIMPString` is converted to UTF-8 using `WideToUTF8`
 * and the COM object is released. If the property cannot be retrieved or the
 * resulting string is empty, the provided `defaultValue` is returned.
 *
 * @param info Pointer to an `IAIMPFileInfo` instance to query. The caller is
 *             responsible for providing a valid pointer (no null check is
 *             performed inside).
 * @param propID Integer identifier of the property to request.
 * @param defaultValue Fallback UTF-8 string returned when the property is
 *                     missing, empty, or retrieval fails.
 * @return std::string The property value converted to UTF-8, or `defaultValue`
 *                    when not available.
 *
 * @remarks
 * - The function relies on the COM-style `GetValueAsObject` method and checks
 *   success with `SUCCEEDED(...)`.
 * - The acquired `IAIMPString*` is released via `Release()` to avoid leaks.
 * - `WideToUTF8` is expected to accept the wide string returned by
 *   `IAIMPString::GetData()` and produce a UTF-8 `std::string`.
 * - This function does not perform thread-synchronization; ensure callers do
 *   not use the same `IAIMPFileInfo` concurrently in unsafe ways.
 */
std::string MyPlugin::GetPropertyText(IAIMPFileInfo* info, int propID, const std::string& defaultValue)
{
    IAIMPString* aString = nullptr;
    std::string result = "";
    if (SUCCEEDED(info->GetValueAsObject(propID, IID_IAIMPString, (void**)&aString)))
    {
        result = WideToUTF8(aString->GetData());
        aString->Release();
    }
    return result.empty() ? defaultValue : result;
}

/**
 * @brief Create an `IAIMPString` containing the UTF-8 text provided.
 *
 * This helper creates an `IAIMPString` COM object via the plugin core and
 * initializes its content using the UTF-8 input `utf8Text`.
 *
 * Behavior:
 * - Calls `_core->CreateObject(IID_IAIMPString, (void**)ppAIMPString)` to
 *   instantiate an `IAIMPString`. If this call fails the function returns
 *   `E_FAIL` and no object is returned.
 * - Converts the UTF-8 `std::string` to a `std::wstring` using
 *   `Utf8ToWide()` and calls `IAIMPString::SetData` to set the wide text and
 *   length (in characters).
 *
 * Threading & COM:
 * - The function uses the `_core` COM interface. Callers must respect COM
 *   apartment/threading rules required by AIMP core services (typically the
 *   AIMP main thread or other threads allowed by AIMP).
 *
 * Ownership / Lifetime:
 * - On success (CreateObject succeeded), `*ppAIMPString` receives a reference
 *   to a newly created `IAIMPString`. The caller is responsible for calling
 *   `Release()` on the returned `IAIMPString*` when no longer needed.
 * - If `CreateObject` fails, no object is returned and nothing needs to be
 *   released.
 * - Note: this function returns the HRESULT from `SetData`. If `CreateObject`
 *   succeeds but `SetData` fails, the created `IAIMPString` is still returned
 *   to the caller and must be released by the caller to avoid a leak.
 *
 * Parameters:
 * - `utf8Text` : UTF-8 encoded text to store in the `IAIMPString`.
 * - `ppAIMPString` : Out parameter that will receive the `IAIMPString*`.
 *
 * Return:
 * - `S_OK` if the string object was created and `SetData` succeeded.
 * - `E_FAIL` if `_core->CreateObject` failed.
 * - Other HRESULT error codes propagated from `SetData` when creation
 *   succeeds but initialization fails.
 *
 * Example:
 * IAIMPString *str = nullptr;
 * if (SUCCEEDED(CreateAIMPString("Hello", &str))) {
 *     // use str ...
 *     str->Release();
 * }
 */
HRESULT MyPlugin::CreateAIMPString(const std::string &utf8Text, IAIMPString **ppAIMPString)
{
    if (FAILED(_core->CreateObject(IID_IAIMPString, (void **)ppAIMPString)))
        return E_FAIL;
    std::wstring wstr = Utf8ToWide(utf8Text);
    return (*ppAIMPString)->SetData((WCHAR *)wstr.c_str(), wstr.length());
}

/**
 * @brief Initialize the plugin and acquire required AIMP services.
 *
 * This function is called by AIMP to initialize the plugin. It performs
 * the following responsibilities:
 * - Validates the provided `IAIMPCore*` pointer and takes ownership by
 *   calling `AddRef()` on `_core`.
 * - Queries and stores pointers to the AIMP services used by the plugin:
 *   `IID_IAIMPServicePlayer`, `IID_IAIMPServiceAlbumArt`,
 *   `IID_IAIMPServicePlaylistManager`, and `IID_IAIMPServiceThreads`.
 *   If any required service query fails the function returns `E_FAIL`.
 * - Attempts to hook into AIMP's message dispatcher (`IID_IAIMPServiceMessageDispatcher`)
 *   — if available — by calling `Hook(this)`. The dispatcher pointer is released
 *   after hooking.
 * - Creates and configures the WebSocket server (`_wsServer`) using the ixwebsocket
 *   wrapper:
 *   - Instantiates `_wsServer` with `Config::WEBSOCKET_PORT` and
 *     `Config::WEBSOCKET_HOST`.
 *   - Sets an on-connection callback that:
 *     - Adds a connected client to the `_wsClients` list on `Open`.
 *     - Removes the client from `_wsClients` on `Close` or `Error`.
 *     - Uses `_wsMutex` to synchronize access to `_wsClients`.
 *   - Calls `listen()` and starts the server if listening succeeded.
 * - Registers HTTP API routes by calling `RegisterPlayerRoutes(this)`,
 *   `RegisterPlaylistRoutes(this)` and `RegisterTrackRoutes(this)`.
 * - Launches the HTTP server thread (`_httpThread`) which:
 *   - Sets `_httpRunning = true`, calls `_httpServer.listen()` with
 *     `Config::HTTP_HOST`/`Config::HTTP_PORT`, and clears `_httpRunning`
 *     when `listen()` returns. The thread is owned by `_httpThread`.
 *
 * Threading & COM notes:
 * - Many AIMP COM interfaces are apartment-threaded. Callers and this
 *   initialization code must observe AIMP/COM threading rules. The function
 *   only queries services and sets up networking; any future use of COM
 *   interfaces must be done on the appropriate thread or via AIMP's thread
 *   service (`_threadService`) if required.
 * - `_core->AddRef()` is called to keep the `IAIMPCore` reference valid for
 *   the plugin lifetime. Corresponding `Release()` must be done in `Finalize()`.
 *
 * Resource ownership and cleanup expectations:
 * - On success the plugin stores pointers in member variables:
 *   `_core`, `_playerService`, `_albumArtService`, `_playlistService`,
 *   `_threadService`, `_wsServer`, `_httpThread`.
 * - The WebSocket server (`_wsServer`) is allocated with `new` and must be
 *   stopped and deleted in `Finalize()`.
 * - The HTTP server runs in `_httpThread` and `_httpRunning` indicates whether
 *   it is active; the thread must be joined and server stopped during finalization.
 *
 * Return values:
 * - `S_OK` on successful initialization.
 * - `E_INVALIDARG` if `Core` is null.
 * - `E_FAIL` if any required service query fails or object creation fails.
 *
 * Example usage:
 * - AIMP calls the plugin entry point which forwards `IAIMPCore*` to this
 *   function. After `S_OK` is returned the plugin is ready to accept
 *   websocket and HTTP clients and respond to route requests.
 *
 * See also:
 * - `Finalize()` for the corresponding cleanup logic.
 * - `RegisterPlayerRoutes`, `RegisterPlaylistRoutes`, `RegisterTrackRoutes`
 *   for the HTTP route handlers registered here.
 */
HRESULT WINAPI MyPlugin::Initialize(IAIMPCore *Core)
{

    if (!Core)
        return E_INVALIDARG;
    _core = Core;
    _core->AddRef();

    if (FAILED(_core->QueryInterface(IID_IAIMPServicePlayer, (void **)&_playerService)))
        return E_FAIL;
    if (FAILED(_core->QueryInterface(IID_IAIMPServiceAlbumArt, (void **)&_albumArtService)))
        return E_FAIL;
    if (FAILED(_core->QueryInterface(IID_IAIMPServicePlaylistManager, (void **)&_playlistService)))
        return E_FAIL;
    if (FAILED(_core->QueryInterface(IID_IAIMPServiceThreads, (void **)&_threadService)))
        return E_FAIL;

    IAIMPServiceMessageDispatcher *dispatcher = nullptr;
    if (SUCCEEDED(_core->QueryInterface(IID_IAIMPServiceMessageDispatcher, (void **)&dispatcher)))
    {
        dispatcher->Hook(this);
        dispatcher->Release();
    }

    _wsServer = new ix::WebSocketServer(Config::WEBSOCKET_PORT, Config::WEBSOCKET_HOST);

    _wsServer->setOnConnectionCallback(
        [this](std::weak_ptr<ix::WebSocket> ws_weak, std::shared_ptr<ix::ConnectionState> state)
        {
            auto ws = ws_weak.lock();
            if (!ws)
                return;

            ws->setOnMessageCallback(
                [this, ws_weak](const ix::WebSocketMessagePtr& msg)
                {
                    auto ws = ws_weak.lock();
                    if (!ws)
                        return;

                    if (msg->type == ix::WebSocketMessageType::Open)
                    {
                        std::lock_guard<std::mutex> lock(_wsMutex);
                        _wsClients.push_back(ws);
                    }
                    else if (msg->type == ix::WebSocketMessageType::Close ||
                        msg->type == ix::WebSocketMessageType::Error)
                    {
                        std::lock_guard<std::mutex> lock(_wsMutex);
                        _wsClients.erase(
                            std::remove_if(_wsClients.begin(), _wsClients.end(),
                                [&ws](const std::shared_ptr<ix::WebSocket>& c)
                                {
                                    return c == ws;
                                }),
                            _wsClients.end());
                    }
                });
        });

    auto wsResult = _wsServer->listen();
    if (wsResult.first)
    {
        _wsServer->start();
    }

    RegisterPlayerRoutes(this);
    RegisterPlaylistRoutes(this);
    RegisterTrackRoutes(this);

    _httpThread = std::make_unique<std::thread>([this]() {
        _httpRunning = true;
        _httpServer.listen(Config::HTTP_HOST, Config::HTTP_PORT);
        _httpRunning = false;
        });

    return S_OK;
}

/**
 * @brief Finalize
 *
 * Perform orderly shutdown and release of resources acquired by the plugin.
 *
 * Responsibilities:
 * - Stop the HTTP server and join the HTTP worker thread if it is running.
 *   - Calls `_httpServer.stop()` to request the server to exit its `listen()` loop.
 *   - If the HTTP thread exists and is joinable, waits for it to finish via `join()`.
 * - Stop and destroy the WebSocket server (`_wsServer`) if it was created.
 *   - Calls `_wsServer->stop()` to terminate listeners/accept loop, then deletes
 *     the heap-allocated server and nulls the pointer.
 * - Clear the list of connected WebSocket clients (`_wsClients`) under the
 *   protection of `_wsMutex` to avoid races with connection callbacks.
 *   - Each client is held by `std::shared_ptr`; clearing the vector allows
 *     those shared pointers to be released so client state can be destroyed.
 * - Unhook from AIMP's `IAIMPServiceMessageDispatcher` (if available).
 *   - Queries the dispatcher via `_core->QueryInterface(...)`. If found, calls
 *     `Unhook(this)` and `Release()` on the dispatcher interface.
 * - Release all acquired AIMP COM service interfaces and the stored `_core`.
 *   - Releases `_playerService`, `_albumArtService`, `_playlistService` and
 *     finally `_core`, nulling each pointer after `Release()` to avoid
 *     dangling references.
 *
 * Threading & COM notes:
 * - This function assumes it is safe to call `stop()` on network servers and
 *   to `join()` the HTTP thread from the current thread. The code uses
 *   `_httpRunning` as an indication that the HTTP server is active before
 *   attempting to stop/join it.
 * - Many AIMP COM interfaces are apartment-threaded. `Finalize()` should be
 *   invoked on the same COM apartment (typically AIMP's main thread) that is
 *   compatible with releasing the interfaces acquired in `Initialize()`.
 *   Releasing COM objects from the wrong thread/apartment can cause undefined
 *   behaviour. The plugin's `Initialize()` documentation explains COM/thread
 *   expectations in more detail.
 *
 * Resource ownership & ordering:
 * - Stop HTTP first so the listening loop returns and the thread can exit.
 * - Stop WebSocket server next and delete the object to free its resources.
 * - Clear client list while holding `_wsMutex` to synchronize with connection
 *   callbacks set up during `Initialize()`.
 * - Unhook from the message dispatcher before releasing `_core` to ensure the
 *   dispatcher will not attempt callbacks into a partially destroyed plugin.
 * - Release service pointers and `_core` last.
 *
 * Return:
 * - Always returns `S_OK`. The function is designed to best-effort stop and
 *   release resources; individual failures are not propagated via the HRESULT.
 */
HRESULT WINAPI MyPlugin::Finalize()
{
    if (_httpRunning)
    {
        _httpServer.stop();
        if (_httpThread && _httpThread->joinable())
        {
            _httpThread->join();
        }
    }

    if (_wsServer)
    {
        _wsServer->stop();
        delete _wsServer;
        _wsServer = nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(_wsMutex);
        _wsClients.clear();
    }

    IAIMPServiceMessageDispatcher *dispatcher = nullptr;
    if (_core && SUCCEEDED(_core->QueryInterface(IID_IAIMPServiceMessageDispatcher, (void **)&dispatcher)))
    {
        dispatcher->Unhook(this);
        dispatcher->Release();
    }

    if (_playerService)
    {
        _playerService->Release();
        _playerService = nullptr;
    }
    if (_albumArtService)
    {
        _albumArtService->Release();
        _albumArtService = nullptr;
    }
    if (_playlistService)
    {
        _playlistService->Release();
        _playlistService = nullptr;
    }
    if (_core)
    {
        _core->Release();
        _core = nullptr;
    }

    return S_OK;
}

void WINAPI MyPlugin::SystemNotification(int NotifyID, IUnknown* Data) {}

// =========================================================================
// QueryInterface
// Must expose IAIMPMessageHook so dispatcher->Hook(this) can QI for it
// internally and route CoreMessage() calls back to us.
// =========================================================================
HRESULT WINAPI MyPlugin::QueryInterface(REFIID riid, void **ppvObject)
{
    if (!ppvObject)
        return E_POINTER;

    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAIMPPlugin))
    {
        *ppvObject = static_cast<IAIMPPlugin *>(this);
        AddRef();
        return S_OK;
    }

    if (IsEqualIID(riid, IID_IAIMPMessageHook))
    {
        *ppvObject = static_cast<IAIMPMessageHook *>(this);
        AddRef();
        return S_OK;
    }

    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG WINAPI MyPlugin::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI MyPlugin::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

/**
 * BroadcastWS
 *
 * Serialize a JSON object and broadcast the resulting text payload to all
 * connected WebSocket clients stored in `_wsClients`.
 *
 * Behavior:
 * - Serializes `msg` using `msg.dump()` (nlohmann::json).
 * - Acquires `_wsMutex` to protect concurrent access to `_wsClients`.
 * - Removes any clients whose socket `getReadyState()` is not `Open`.
 * - Iterates the remaining clients and calls `send(text)` on each.
 *
 * Thread-safety:
 * - Access to `_wsClients` is synchronized with `_wsMutex` for the duration
 *   of the cleanup + send loop. This prevents race conditions with other
 *   threads that may add/remove clients.
 *
 * Notes / Considerations:
 * - `ix::WebSocket::send` semantics depend on the ixwebsocket library
 *   configuration (it may perform internal queueing or block). If sending
 *   large messages or if client send may block, consider sending
 *   asynchronously or offloading to a worker thread to avoid blocking the
 *   caller.
 * - If `send` can throw, wrap calls in try/catch to avoid terminating the
 *   broadcasting loop.
 *
 * Parameters:
 * - msg: const json& - message to broadcast (expects nlohmann::json).
 */
void MyPlugin::BroadcastWS(const json &msg)
{
    std::string text = msg.dump();
    std::lock_guard<std::mutex> lock(_wsMutex);

    // Remove non-open clients
    _wsClients.erase(
        std::remove_if(_wsClients.begin(), _wsClients.end(),
            [](const std::shared_ptr<ix::WebSocket>& c)
            {
                return c->getReadyState() != ix::ReadyState::Open;
            }),
        _wsClients.end());

    // Send to remaining connected clients
    for (auto& client : _wsClients)
    {
        client->send(text);
    }
}

// =========================================================================
// CoreMessage
// Called by AIMP on the main thread for every internal event.
// We filter the events we care about and broadcast JSON to WS clients.
// =========================================================================
void WINAPI MyPlugin::CoreMessage(LongWord AMessage, int AParam1, void *AParam2, HRESULT *AResult)
{
    switch (AMessage)
    {

    // ---------------------------------------------------------------------
    // New track started (or internet radio changed sub-track).
    // Read the current file info from IAIMPServicePlayer and broadcast it.
    // ---------------------------------------------------------------------
    case AIMP_MSG_EVENT_STREAM_START:
    case AIMP_MSG_EVENT_STREAM_START_SUBTRACK:
    {
        json info = BuildTrackInfoJson();
        if (!info.empty())
        {
            info["event"] = "track_changed";
            BroadcastWS(info);
        }
        break;
    }

    // ---------------------------------------------------------------------
    // Player state changed: stopped (0), paused (1), playing (2).
    // AParam1 carries the new state directly.
    // ---------------------------------------------------------------------
    case AIMP_MSG_EVENT_PLAYER_STATE:
    {
        BroadcastWS({{"event", "player_state"},
                     {"state", AParam1}});
        break;
    }

    // ---------------------------------------------------------------------
    // Position timer tick — fires every second while playing.
    // AParam1/AParam2 are unused; we query the real position ourselves.
    // ---------------------------------------------------------------------
    case AIMP_MSG_EVENT_PLAYER_UPDATE_POSITION:
    {
        if (_playerService)
        {
            HWND hAIMP = FindWindowA(AIMPRemoteAccessClass, NULL);
            if (hAIMP)
            {
                LRESULT position = SendMessage(hAIMP, WM_AIMP_PROPERTY, AIMP_RA_PROPERTY_PLAYER_POSITION, 0);
                BroadcastWS({{"event", "position"},
                             {"position", position}});
            }
        }
        break;
    }

    // ---------------------------------------------------------------------
    // A property changed.
    // AParam1 = property ID (AIMP_MSG_PROPERTY_XXX)
    // AParam2 = pointer to the new value (type depends on the property)
    // ---------------------------------------------------------------------
    case AIMP_MSG_EVENT_PROPERTY_VALUE:
    {
        switch (AParam1)
        {

            // User seeked to a new position manually.
            // AParam2 → float* (seconds)
        case AIMP_MSG_PROPERTY_PLAYER_POSITION:
        {
            float pos = *static_cast<float *>(AParam2);
            BroadcastWS({{"event", "position"},
                         {"position", static_cast<double>(pos)},
                         {"seeked", true}});
            break;
        }

        // Mute toggled.
        // AParam2 → DWORD* (LongBool: 0 = unmuted, non-zero = muted)
        case AIMP_MSG_PROPERTY_MUTE:
        {
            bool muted = (*static_cast<DWORD *>(AParam2)) != 0;
            BroadcastWS({{"event", "mute_changed"},
                         {"mute", muted}});
            break;
        }

        // Volume changed.
        // AParam2 → float* in [0.0 .. 1.0]; convert to 0..100.
        case AIMP_MSG_PROPERTY_VOLUME:
        {
            float vol = *static_cast<float *>(AParam2);
            BroadcastWS({{"event", "volume_changed"},
                         {"volume", static_cast<int>(vol * 100.0f + 0.5f)}});
            break;
        }

        // Shuffle toggled.
        // AParam2 → DWORD* (LongBool)
        case AIMP_MSG_PROPERTY_SHUFFLE:
        {
            bool on = (*static_cast<DWORD *>(AParam2)) != 0;
            BroadcastWS({{"event", "shuffle_changed"},
                         {"shuffle", on}});
            break;
        }

        // Repeat toggled.
        // AParam2 → DWORD* (LongBool)
        case AIMP_MSG_PROPERTY_REPEAT:
        {
            bool on = (*static_cast<DWORD *>(AParam2)) != 0;
            BroadcastWS({{"event", "repeat_changed"},
                         {"repeat", on}});
            break;
        }

        } // inner switch (AParam1)
        break;
    }

    } // outer switch (AMessage)
}

TChar *WINAPI MyPlugin::InfoGet(int Index)
{
    return ::InfoGet(Index);
}

LongWord WINAPI MyPlugin::InfoGetCategories()
{
    return ::InfoGetCategories();
}
