#pragma once

#include "../core/Plugin.h"
#include "../helpers/TrackInfoHelper.h"
#include <nlohmann/json.hpp>

/**
 * @brief Task that collects available playlists from the plugin.
 *
 * This class implements the IAIMPTask interface so it can be scheduled on
 * AIMP's task runner. The task queries the provided `MyPlugin` instance
 * to build a list of `PlaylistData` entries and stores them in `_results`.
 *
 * Threading / lifetime notes:
 * - The task is reference-counted using COM-style `AddRef` / `Release`.
 * - `Execute` runs on the worker thread used by the task runner. Callers
 *   must ensure synchronization when accessing results (see `GetResults`).
 */
class CGetTrackInfoTask : public IAIMPTask
{
private:
    /**
     * @brief COM-style reference count.
     *
     * Starts at 1 for the object owner. Marked `volatile` because it may be
     * modified from different threads via `AddRef` / `Release`.
     */
    volatile ULONG _refCount = 1;

    /**
     * @brief Non-owning pointer to the plugin instance used to retrieve playlists.
     *
     * The plugin must outlive this task.
     */
    MyPlugin* _plugin;

    
    nlohmann::json _result;

public:
    /**
     * @brief Constructs the task.
     * @param plugin Pointer to the plugin used to query playlists. Must not be null.
     */
    CGetTrackInfoTask(MyPlugin* plugin);

    /**
     * @brief COM-style QueryInterface implementation.
     * @param riid Reference to the requested interface ID.
     * @param ppvObject Receives the interface pointer on success.
     * @return HRESULT indicating success or failure (standard COM codes).
     */
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void** ppvObject) override;

    /**
     * @brief Increment the reference count.
     * @return The new reference count.
     */
    virtual ULONG WINAPI AddRef() override;

    /**
     * @brief Decrement the reference count and delete when it reaches zero.
     * @return The new reference count.
     */
    virtual ULONG WINAPI Release() override;

    /**
     * @brief Task entry point called by the task runner.
     * @param Owner Task owner provided by the host; may be used to notify about progress or
     *              cancellation. Implementation should use `Owner` according to IAIMPTask docs.
     *
     * Expected behavior: query `_plugin` for playlists and populate `_results`.
     */
    virtual void WINAPI Execute(IAIMPTaskOwner* Owner) override;

    const nlohmann::json& GetResult() const { return _result; }
};