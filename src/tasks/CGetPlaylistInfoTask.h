#pragma once

#include "../core/Plugin.h"
#include "../models/Playlist.h"

/**
 * @file CGetPlaylistInfoTask.h
 * @brief Declaration of `CGetPlaylistInfoTask` - a small IAIMPTask implementation
 *        used to fetch fast information about a playlist (identified by id).
 *
 * This task is intended to be posted to AIMP's task system. When executed it
 * retrieves lightweight/fast playlist metadata and stores it in `_result`
 * so callers can obtain it via `GetResult()` once the task has completed.
 */
class CGetPlaylistInfoTask : public IAIMPTask
{
private:
    /**
     * COM-style reference count. Marked `volatile` because it can be
     * modified from multiple threads (AddRef/Release).
     */
    volatile ULONG _refCount = 1;

    /**
     * Pointer to the plugin instance that created the task. Not owned by
     * this task (the plugin must outlive the task or ensure proper lifetime).
     */
    MyPlugin* _plugin;

    /**
     * The playlist identifier used to query the playlist information.
     */
    std::string _playlistId;

    /**
     * Storage for the retrieved fast playlist information. Populated by
     * `Execute(...)` and accessible via `GetResult()`.
     */
    PlaylistInfoFastData _result;

public:
    /**
     * @brief Construct a new CGetPlaylistInfoTask.
     * @param plugin Pointer to the creating `MyPlugin` instance (not owned).
     * @param id The playlist identifier to query.
     */
    CGetPlaylistInfoTask(MyPlugin *plugin, const std::string &id);

    /**
     * @name COM IUnknown / IAIMPTask interface
     * These follow COM semantics: callers should use `AddRef`/`Release` to
     * manage the lifetime of the task object.
     */
     ///@{
     /**
      * @brief Standard COM QueryInterface implementation.
      * @param riid The interface ID being requested.
      * @param ppvObject Receives the interface pointer on success.
      * @return HRESULT S_OK on success or an error code otherwise.
      */
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **ppvObject) override;

    /**
     * @brief Increment the reference count.
     * @return The new reference count.
     */
    virtual ULONG WINAPI AddRef() override;

    /**
     * @brief Decrement the reference count and delete the object when it
     *        reaches zero.
     * @return The new reference count.
     */
    virtual ULONG WINAPI Release() override;

    /**
     * @brief Task entry point invoked by the AIMP task runner.
     * @param Owner The task owner provided by the AIMP host (may be nullptr).
     *
     * Expected behavior: perform a fast query for playlist metadata identified
     * by `_playlistId` and store the result into `_result`. This method runs
     * on a worker thread and must be thread-safe.
     */
    virtual void WINAPI Execute(IAIMPTaskOwner *Owner) override;

    /**
     * @brief Get the fast playlist information result populated by `Execute`.
     * @return const PlaylistInfoFastData& Read-only reference to the result.
     *
     * Note: callers must ensure `Execute` has finished before reading the
     * result (synchronization is the caller's responsibility).
     */
    const PlaylistInfoFastData &GetResult() const { return _result; }
};