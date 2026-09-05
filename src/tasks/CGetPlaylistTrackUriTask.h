#pragma once

#include "../models/Playlist.h"
#include "../core/Plugin.h"

/**
 * @class CGetPlaylistTrackUriTask
 * @brief Task that retrieves the URI for a specific track in a playlist.
 *
 * This class implements the IAIMPTask interface and is used to query AIMP for
 * the URI of a given track within a specific playlist.
 *
 * Threading / lifetime notes:
 * - Implements COM-style reference counting via `AddRef`/`Release`.
 * - The task is executed through AIMP's task system; `Execute` runs on the
 *   AIMP thread context supplied by the host (caller).
 * - All COM interfaces obtained inside `Execute` are released before returning.
 */
class CGetPlaylistTrackUriTask : public IAIMPTask
{
private:
    /**
     * @brief COM reference count (volatile for atomic ops).
     *
     * Initialized to 1 to represent the creator's reference.
     */
    volatile ULONG _refCount = 1;

    /**
     * @brief Non-owning pointer to the main plugin instance.
     *
     * Used to access AIMP services and helper methods. The plugin instance is
     * expected to outlive this task.
     */
    MyPlugin* _plugin;

    /**
     * @brief Playlist ID for which to retrieve the track URI.
     *
     * This UTF-8 string identifies the target playlist within AIMP. It is
     * provided at construction time and used by `Execute` to locate the
     * corresponding playlist. This class does not own external memory for the
     * string; it stores its own copy.
     */
    std::string _playlistId;
    
    /**
     * @brief Collected file URIs for the playlist's tracks.
     *
     * After `Execute` runs, this vector contains UTF-8 encoded strings for each
     * track's URI (for example "file://...", "http://...", etc.). The vector
     * is owned by the task instance.
     *
     * Ownership and lifetime:
     * - The task owns `_fileUris` and is responsible for its contents.
     * - Callers may obtain a const reference via `GetFileUris()`; they MUST NOT
     *   modify the returned container.
     *
     * Thread-safety:
     * - `_fileUris` is populated during `Execute`. Access from other threads
     *   while `Execute` is running requires external synchronization.
     */
    std::vector<std::string> _fileUris;

public:
    /**
     * @brief Constructs the task with a reference to the plugin.
     * @param plugin Pointer to `MyPlugin` that provides access to AIMP services.
     *
     * The constructor does not take ownership of `plugin`.
     */
    CGetPlaylistTrackUriTask(MyPlugin* plugin, const std::string& playlistId);

    /**
     * @brief Standard COM QueryInterface implementation.
     * @param riid Interface ID requested.
     * @param ppvObject Receives interface pointer on success.
     * @return S_OK if supported, E_NOINTERFACE or E_POINTER otherwise.
     *
     * On success this method calls `AddRef()` on the returned interface.
     */
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObject) override;

    /**
     * @brief Increments the COM reference count.
     * @return The new reference count.
     */
    virtual ULONG WINAPI AddRef() override;

    /**
     * @brief Decrements the COM reference count and deletes the object at zero.
     * @return The new reference count.
     */
    virtual ULONG WINAPI Release() override;

    /**
     * @brief Executes the task to obtain the current playlist.
     * @param Owner Task owner provided by AIMP (may be nullptr or unused).
     *
     * Execution steps (high level):
     *  - Acquire the player service from the plugin/core.
     *  - Ask the player for the currently playing `IAIMPPlaylistItem`.
     *  - From the item obtain the parent `IAIMPPlaylist`.
     *  - Read playlist properties (id, name) and item count into `_result`.
     *  - Set `_result.found = true` if a playlist was read successfully.
     *
     * All COM interfaces acquired during execution are released before return.
     */
    virtual void WINAPI Execute(IAIMPTaskOwner* Owner) override;

    /**
     * @brief Returns the list of file URIs collected by this task.
     *
     * The vector contains UTF-8 encoded strings representing the tracks' URIs
     * (for example "file://...", "http://...", etc.). The contents are
     * populated when `Execute` runs and queries AIMP for the playlist items.
     *
     * Ownership and lifetime:
     * - The returned reference refers to the task's internal storage (`_fileUris`).
     * - The caller MUST NOT attempt to modify the returned vector.
     * - The reference is valid only while the task object remains alive and no
     *   non-const operations are performed on the task.
     *
     * Thread-safety:
     * - This accessor is `const` but is not synchronized internally.
     * - It is safe to call from the caller thread after the task has completed
     *   execution. Concurrent access while `Execute` is running is not safe
     *   unless the caller enforces external synchronization.
     *
     * @return const std::vector<std::string>& Reference to the internal list of file URIs.
     */
    const std::vector<std::string>& GetFileUris() const { return _fileUris; }
};