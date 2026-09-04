#pragma once
#include "../core/Plugin.h"

/**
 * @class CDeletePlaylistItemTask
 * @brief Task that deletes an item from a specific playlist.
 *
 * This class implements the IAIMPTask interface and is used to query AIMP for
 * deleting a given item within a specific playlist.
 *
 * Threading / lifetime notes:
 * - Implements COM-style reference counting via `AddRef`/`Release`.
 * - The task is executed through AIMP's task system; `Execute` runs on the
 *   AIMP thread context supplied by the host (caller).
 * - All COM interfaces obtained inside `Execute` are released before returning.
 */
class CDeletePlaylistItemTask : public IAIMPTask
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

    
    std::vector<int> _songsIndexes;

    /**
    * @brief Controls whether the deletion is physical (removes file) or logical (removes from playlist only).
    *
    * When `true` the task should attempt a physical delete of the underlying
    * media resource (for example, remove the file from disk or invoke the
    * host's physical-delete API). When `false` the task must only remove the
    * item from the playlist without touching the underlying media object.
    *
    * Notes:
    * - The exact behavior for a "physical" delete depends on the host AIMP APIs
    *   and permissions; `Execute` should perform necessary checks and fall back
    *   to playlist-only removal if a physical delete is not possible.
    * - Default/initial value: `false` (do not delete files) unless explicitly set.
    * - Thread-safety: treated as read-only by `Execute`. The creator must set
    *   this value before scheduling the task and must not modify it while the
    *   task executes.
    */
    boolean _physicalDelete;

    bool _hasErrors = false;

public:
    /**
     * @brief Constructs the task with a reference to the plugin.
     * @param plugin Pointer to `MyPlugin` that provides access to AIMP services.
     *
     * The constructor does not take ownership of `plugin`.
     */
    CDeletePlaylistItemTask(MyPlugin* plugin, const std::string& _playlistId, const std::vector<int>& _songsIndexes, boolean _physicalDelete);

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

    bool HasErrors() const { return _hasErrors; }

};