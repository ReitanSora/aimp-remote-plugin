#pragma once

#include "../models/Playlist.h"
#include "../core/Plugin.h"

/**
 * @class CGetCurrentPlaylistTask
 * @brief Task that retrieves metadata for the currently active playlist.
 *
 * This class implements the IAIMPTask interface and is used to query AIMP for
 * the playlist that contains the currently playing item. The retrieved data
 * is stored in a `CurrentPlaylistData` structure accessible via `GetResult()`.
 *
 * Threading / lifetime notes:
 * - Implements COM-style reference counting via `AddRef`/`Release`.
 * - The task is executed through AIMP's task system; `Execute` runs on the
 *   AIMP thread context supplied by the host (caller).
 * - All COM interfaces obtained inside `Execute` are released before returning.
 */
class CGetCurrentPlaylistTask : public IAIMPTask
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
    MyPlugin *_plugin;

    /**
     * @brief Result container filled by `Execute`.
     *
     * `found` will be set to true when a playlist is successfully read.
     */
    CurrentPlaylistData _result;

public:
    /**
     * @brief Constructs the task with a reference to the plugin.
     * @param plugin Pointer to `MyPlugin` that provides access to AIMP services.
     *
     * The constructor does not take ownership of `plugin`.
     */
    CGetCurrentPlaylistTask(MyPlugin *plugin);

    /**
     * @brief Standard COM QueryInterface implementation.
     * @param riid Interface ID requested.
     * @param ppvObject Receives interface pointer on success.
     * @return S_OK if supported, E_NOINTERFACE or E_POINTER otherwise.
     *
     * On success this method calls `AddRef()` on the returned interface.
     */
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **ppvObject) override;

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
    virtual void WINAPI Execute(IAIMPTaskOwner *Owner) override;

    /**
     * @brief Returns the data gathered by the last `Execute` call.
     * @return Constant reference to `CurrentPlaylistData`.
     *
     * The returned reference is only valid while this object exists.
     */
    const CurrentPlaylistData &GetResult() const { return _result; }
};