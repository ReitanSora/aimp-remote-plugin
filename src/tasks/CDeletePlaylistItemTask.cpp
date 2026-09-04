#include "pch.h"
#include "CDeletePlaylistItemTask.h"

static BOOL CALLBACK DeleteItemsCallback(IAIMPPlaylistItem* Item, void* UserData)
{
    auto* itemsToDelete = static_cast<std::unordered_set<IAIMPPlaylistItem*>*>(UserData);

    if (itemsToDelete->find(Item) != itemsToDelete->end())
    {
        return TRUE;
    }
    return FALSE;
}

CDeletePlaylistItemTask::CDeletePlaylistItemTask(MyPlugin* plugin, const std::string& _playlistId, const std::vector<int>& _songsIndexes, boolean _physicalDelete)
    : _plugin(plugin), _playlistId(_playlistId), _songsIndexes(_songsIndexes), _physicalDelete(_physicalDelete) {
}

HRESULT WINAPI CDeletePlaylistItemTask::QueryInterface(REFIID riid, void** ppvObject)
{
    if (!ppvObject)
        return E_POINTER;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAIMPTask))
    {
        *ppvObject = this;
        AddRef();
        return S_OK;
    }
    *ppvObject = nullptr;
    return E_NOINTERFACE;
}

ULONG WINAPI CDeletePlaylistItemTask::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI CDeletePlaylistItemTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CDeletePlaylistItemTask::Execute(IAIMPTaskOwner* Owner)
{
	IAIMPServicePlaylistManager* manager = _plugin->GetPlaylistService();
    if (!manager) return;

	IAIMPString* idPlaylist = nullptr;
	if (FAILED(_plugin->CreateAIMPString(_playlistId, &idPlaylist))) return;

	IAIMPPlaylist* playlist = nullptr;
	if (FAILED(manager->GetLoadedPlaylistByID(idPlaylist, &playlist)))
	{
		idPlaylist->Release();
		return;
	}

    std::unordered_set<IAIMPPlaylistItem*> itemsToDelete;
    int maxItems = playlist->GetItemCount();

    for (int idx : _songsIndexes)
    {
        if (idx >= 0 && idx < maxItems)
        {
            IAIMPPlaylistItem* item = nullptr;
            if (SUCCEEDED(playlist->GetItem(idx, IID_IAIMPPlaylistItem, (void**)&item)))
            {
                itemsToDelete.insert(item);
            }
            else
            {
                _hasErrors = true;
            }
        }
        else
        {
            _hasErrors = true;
        }
    }

    if (itemsToDelete.empty())
    {
        playlist->Release();
        return;
    }

	playlist->BeginUpdate();

    if (_physicalDelete)
    {
		LongWord flags = AIMP_PLAYLIST_DELETE_FLAGS_PHYSICALLY | AIMP_PLAYLIST_DELETE_FLAGS_NOCONFIRMATION;
        playlist->Delete3(flags, DeleteItemsCallback, &itemsToDelete);
    }
    else
    {
        for (auto* item : itemsToDelete) 
        {
            playlist->Delete(item);
        }
    }

	playlist->EndUpdate();

	for (auto* item : itemsToDelete)
	{
		item->Release();
	}

	playlist->Release();

}