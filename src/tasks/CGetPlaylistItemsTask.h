#pragma once
#include "../models/Track.h"
#include "../core/Plugin.h"

/**
 * @file CGetPlaylistItemsTask.h
 * @brief Task used to fetch items from an AIMP playlist asynchronously.
 *
 * This class implements the `IAIMPTask` interface so it can be scheduled via
 * AIMP's thread service. It queries the playlist identified by `playlistId`
 * and fills a `std::vector<SongData>` with metadata for each track.
 *
 * Threading and ownership notes:
 * - The `MyPlugin* _plugin` pointer is a non-owning (raw) pointer to the plugin
 *   instance; the plugin must outlive this task.
 * - `Execute` is invoked by AIMP's worker threads (via `IAIMPServiceThreads`)
 *   and must only call thread-safe plugin helpers. Any UI or main-thread-only
 *   operations must be marshalled back to the main thread by the plugin.
 * - Reference counting is implemented via COM-style `AddRef`/`Release`.
 */
class CGetPlaylistItemsTask : public IAIMPTask
{
private:
    /**
     * @brief COM-style reference count for the task object.
     *
     * Initialized to 1 when the task is created. `AddRef`/`Release` manage the
     * lifetime. Marked `volatile` because it may be modified from multiple
     * threads.
     */
    volatile ULONG _refCount = 1;

    /**
     * @brief Non-owning pointer to the main plugin instance.
     *
     * The task uses `MyPlugin` services (playlist, core, etc.) to retrieve
     * playlist contents. The plugin must remain valid for the task's lifetime.
     */
    MyPlugin* _plugin;

    /**
     * @brief Playlist identifier to query.
     *
     * Stored as `std::string` (UTF-8). The identifier is copied on construction
     * so the caller can free its own memory immediately after creating the
     * task.
     */
    std::string _playlistId;

    /**
     * @brief Results collected by `Execute`.
     *
     * Filled with `SongData` entries representing each playlist item. Use
     * `GetResults()` to access after the task finishes.
     */
    std::vector<SongData> _results;

public:
    /**
     * @brief Constructs a task to get items from the playlist `id`.
     * @param plugin Non-owning pointer to the `MyPlugin` instance.
     * @param id Playlist identifier (copied).
     */
    CGetPlaylistItemsTask(MyPlugin *plugin, const std::string &id);

    /**
     * @brief Standard COM QueryInterface implementation.
     * @param riid Requested interface ID.
     * @param ppvObject Receives the interface pointer on success.
     * @return HRESULT code (S_OK if interface is supported).
     */
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **ppvObject) override;

    /**
     * @brief Increment reference count.
     * @return New reference count.
     */
    virtual ULONG WINAPI AddRef() override;

    /**
     * @brief Decrement reference count and delete when it reaches zero.
     * @return New reference count.
     */
    virtual ULONG WINAPI Release() override;

    /**
     * @brief Main task entry point invoked by AIMP's thread service.
     *
     * This method should:
     * - Use `_plugin`'s playlist services to enumerate items for `_playlistId`.
     * - Populate `_results` with `SongData` for each entry.
     *
     * Note: `Execute` runs on a worker thread — avoid touching UI-only APIs
     * and ensure thread-safety when interacting with `MyPlugin`.
     *
     * @param Owner Task owner provided by AIMP (can be used to report progress).
     */
    virtual void WINAPI Execute(IAIMPTaskOwner *Owner) override;

    /**
     * @brief Accessor for the collected results.
     * @return Const reference to the vector of `SongData`.
     *
     * Call this after the task has finished executing. The caller is
     * responsible for ensuring the task has completed (e.g., via synchronization
     * with the `IAIMPTaskOwner` or the thread service) before reading results.
     */
    const std::vector<SongData> &GetResults() const { return _results; }
};