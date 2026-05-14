#pragma once

#include "../core/Plugin.h"

/**
 * @file CPlayItemTask.h
 * @brief Task object used to request playback of a specific item inside a playlist.
 *
 * This class implements the IAIMPTask interface so it can be scheduled/executed
 * by AIMP's thread service. The task holds a reference to the owning plugin and
 * the target playlist identifier + item index. When executed on the AIMP main
 * thread, it should instruct AIMP to select/play the requested item using the
 * plugin's services.
 *
 * Lifetime and thread-safety:
 * - Reference counting follows COM-style semantics via AddRef/Release.
 * - The task object may be created on any thread but Execute(...) will be
 *   invoked by AIMP on its worker/main thread. Interactions with AIMP services
 *   must therefore happen from that thread (the implementation should already
 *   use the plugin's thread service when scheduling if needed).
 */
class CPlayItemTask : public IAIMPTask
{
private:
    /**
     * @brief COM-style reference counter.
     *
     * Initialized to 1 when the task is created. Incremented/decremented via
     * `AddRef()`/`Release()`. Marked `volatile` because it can be touched from
     * different threads (creation thread vs AIMP thread).
     */
    volatile ULONG _refCount = 1;

    /**
     * @brief Non-owning pointer to the plugin instance that created this task.
     *
     * Used to access AIMP services such as playlist and player services.
     * The plugin is expected to outlive the task or ensure proper ordering.
     */
    MyPlugin *_plugin;

    /**
     * @brief Identifier of the playlist that contains the item to play.
     *
     * Format and semantics match how playlists are referenced in the plugin
     * (typically an internal playlist ID or GUID stored as a UTF-8 string).
     */
    std::string _playlistId;

    /**
     * @brief Zero-based index of the item inside the playlist to play.
     */
    int _index;

public:
    /**
     * @brief Constructs a play-item task.
     * @param plugin Pointer to the owning `MyPlugin` instance (non-owning).
     * @param id UTF-8 playlist identifier identifying the target playlist.
     * @param index Zero-based item index within the playlist to play.
     *
     * The constructor does not start playback by itself; it only packages the
     * parameters to be executed later on AIMP's thread via `Execute(...)`.
     */
    CPlayItemTask(MyPlugin *plugin, const std::string &id, int index);

    /**
     * @brief IUnknown::QueryInterface implementation.
     * @param riid Reference to the requested interface IID.
     * @param ppvObject Receives the interface pointer on success.
     * @return Standard HRESULT (S_OK, E_NOINTERFACE, etc.).
     */
    virtual HRESULT WINAPI QueryInterface(REFIID riid, void **ppvObject) override;

    /**
     * @brief IUnknown::AddRef implementation.
     * @return The new reference count.
     */
    virtual ULONG WINAPI AddRef() override;

    /**
     * @brief IUnknown::Release implementation.
     * Decrements the reference count and deletes the object when it reaches 0.
     * @return The new reference count.
     */
    virtual ULONG WINAPI Release() override;

    /**
     * @brief IAIMPTask::Execute implementation.
     * @param Owner The task owner provided by AIMP (may be used for cancelation/state).
     *
     * Called by AIMP when the task is executed. The implementation should:
     * - Use `_plugin` to obtain the playlist/player services.
     * - Select the playlist identified by `_playlistId` and start playback of
     *   the item at `_index`.
     *
     * Note: All interactions with AIMP COM services must follow AIMP threading
     * requirements (Execute is expected to run on the AIMP thread).
     */
    virtual void WINAPI Execute(IAIMPTaskOwner *Owner) override;
};