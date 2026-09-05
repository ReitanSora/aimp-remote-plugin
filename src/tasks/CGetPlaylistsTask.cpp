#include "pch.h"
#include "CGetPlaylistsTask.h"


CGetPlaylistsTask::CGetPlaylistsTask(MyPlugin *plugin) : _plugin(plugin) {}

HRESULT WINAPI CGetPlaylistsTask::QueryInterface(REFIID riid, void **ppvObject)
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

ULONG WINAPI CGetPlaylistsTask::AddRef()
{
    return InterlockedIncrement(&_refCount);
}

ULONG WINAPI CGetPlaylistsTask::Release()
{
    ULONG count = InterlockedDecrement(&_refCount);
    if (count == 0)
        delete this;
    return count;
}

void WINAPI CGetPlaylistsTask::Execute(IAIMPTaskOwner *Owner)
{
    if (!_plugin) return;

    IAIMPCore *core = _plugin->GetCore();
    IAIMPServicePlaylistManager *manager = nullptr;

    if (SUCCEEDED(core->QueryInterface(IID_IAIMPServicePlaylistManager, (void **)&manager)))
    {
        int count = manager->GetLoadedPlaylistCount();

        IAIMPPlaylist *activePlaylist = nullptr;
        HRESULT hrActive = manager->GetActivePlaylist(&activePlaylist);

        if (count > 0)
        {
            for (int i = 0; i < count; i++)
            {
                IAIMPPlaylist *pl = nullptr;
                if (SUCCEEDED(manager->GetLoadedPlaylist(i, &pl)))
                {
                    IAIMPPropertyList *propList = nullptr;
                    if (SUCCEEDED(pl->QueryInterface(IID_IAIMPPropertyList, (void **)&propList)))
                    {
                        PlaylistData d;

                        d.name = _plugin->GetPropertyText(propList, AIMP_PLAYLIST_PROPID_NAME, "Unknown name");
                        d.id = _plugin->GetPropertyText(propList, AIMP_PLAYLIST_PROPID_ID, std::to_string(i));
                        d.itemCount = pl->GetItemCount();

                        _results.push_back(d);

                        propList->Release();
                    }
                    pl->Release();
                }
            }
        }
        else
        {
            _results.push_back({"ERROR", "Manager did not found any playlist", 0});
        }

        manager->Release();
    }
}