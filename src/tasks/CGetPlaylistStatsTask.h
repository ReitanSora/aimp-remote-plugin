#pragma once

#include "../core/Plugin.h"
#include "../models/Playlist.h"

/// @brief Task that collects statistical information for a playlist.
/// 
/// This class implements the IAIMPTask interface so it can be scheduled by AIMP's
/// task runner. The task locates a loaded playlist by its ID and iterates its
/// items to gather statistics such as unique genres/artists/albums, average
/// bitrate, average rating, total play count, total size and track counts.
/// 
/// Threading / lifetime:
/// - This object uses COM-style reference counting (AddRef/Release). The initial
///   reference count is 1.
/// - The task may be queried for interfaces via QueryInterface.
/// - Execute runs on the AIMP task thread; callers should check the task owner
///   for cancellation via the provided Owner pointer.
/// 
class CGetPlaylistStatsTask : public IAIMPTask
{
private:
    /// COM reference count (volatile for Interlocked operations). Initialized to 1.
    volatile ULONG _refCount = 1;

    /// Non-owning pointer to the plugin instance. Must remain valid for the task lifetime.
    MyPlugin* _plugin;

    /// Playlist identifier used to locate the loaded playlist.
    std::string _playlistId;

    /// Result structure filled by Execute().
    PlaylistStatsData _result;

public:
    /// @brief Constructs the task.
    /// @param plugin Pointer to the plugin instance (non-owning).
    /// @param id Playlist ID string used to find the loaded playlist.
    CGetPlaylistStatsTask(MyPlugin *plugin, const std::string &id);

    /// @name IUnknown / IAIMPTask (COM) methods
    /// @{
    /// @brief Standard COM QueryInterface implementation.
    /// @param riid Interface ID requested.
    /// @param ppvObject Receives the requested interface pointer on success.
    /// @return S_OK on success, E_POINTER if ppvObject is null, E_NOINTERFACE if the
    ///         interface is not supported.
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **ppvObject) override;

    /// @brief Increments the object's reference count.
    /// @return The new reference count after increment.
    virtual ULONG WINAPI AddRef() override;

    /// @brief Decrements the object's reference count and deletes the object when it reaches zero.
    /// @return The reference count after decrement.
    virtual ULONG WINAPI Release() override;

    /// @brief Entry point executed by AIMP to perform the task work.
    /// @details Execute will:
    ///  - Convert the playlist ID to an AIMP string.
    ///  - Retrieve the loaded playlist by ID.
    ///  - Iterate playlist items and gather metadata from the item's file info.
    ///  - Respect cancellation by periodically checking Owner->IsCanceled().
    ///  - Populate the internal _result structure and mark it as found on success.
    /// @param Owner Optional task owner used to detect cancellation.
    virtual void WINAPI Execute(IAIMPTaskOwner *Owner) override;

    /// @brief Returns the collected playlist statistics.
    /// @note Valid after Execute() has completed. If Execute failed or no playlist
    ///       was found, fields in the returned structure may be unset and
    ///       `found` will be false.
    /// @return Const reference to the PlaylistStatsData result.
    const PlaylistStatsData &GetResult() const { return _result; }
};