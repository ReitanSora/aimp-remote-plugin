#include "pch.h"
#include <nlohmann/json.hpp>
#include "PlaylistRoutes.h"
#include "../core/Plugin.h"
#include "../tasks/CGetPlaylistsTask.h"
#include "../tasks/CGetPlaylistInfoTask.h"
#include "../tasks/CGetPlaylistItemsTask.h"
#include "../tasks/CGetPlaylistStatsTask.h"
#include "../tasks/CGetCurrentPlaylistTask.h"
#include "../tasks/CPlayItemTask.h"

using json = nlohmann::json;

// =============================================================================
// Route Handlers
// =============================================================================

static void HandleGetPlaylistList(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{

    if (!plugin->GetPlaylistService() || !plugin->GetThreadService())
    {
        res.status = 500;
        return;
    }

    CGetPlaylistsTask *task = new CGetPlaylistsTask(plugin);
    task->AddRef();

    HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

    if (SUCCEEDED(hr))
    {
        json response = json::array();

        const auto &results = task->GetResults();

        for (const auto &pl : results)
        {
            response.push_back({{"id", pl.id},
                                {"name", pl.name},
                                {"itemCount", pl.itemCount}});
        }

        res.status = 200;
        res.set_content(response.dump(), "application/json; charset=utf-8");
    }
    else
    {
        res.status = 500;
        res.set_content("{\"error\": \"ExecuteInMainThread Failed\"}", "application/json");
    }

    task->Release();
}

static void HandleGetCurrentPlaylist(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    if (!plugin->GetPlaylistService() || !plugin->GetThreadService())
    {
        res.status = 500;
        res.set_content("{\"error\":\"Services unavailable\"}", "application/json");
        return;
    }

    CGetCurrentPlaylistTask *task = new CGetCurrentPlaylistTask(plugin);
    task->AddRef();

    HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

    if (SUCCEEDED(hr))
    {
        const auto &result = task->GetResult();
        if (result.found)
        {
            res.status = 200;
            res.set_content(json{
                                {"id", result.id},
                                {"name", result.name},
                                {"itemCount", result.itemCount}}
                                .dump(),
                            "application/json; charset=utf-8");
        }
        else
        {
            res.status = 204;
        }
    }
    else
    {
        res.status = 500;
        res.set_content("{\"error\":\"ExecuteInMainThread Failed\"}", "application/json");
    }

    task->Release();
}

static void HandleGetPlaylistInfo(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    std::string id = req.get_param_value("id");
    if (id.empty())
    {
        res.status = 400;
        res.set_content("{\"error\":\"Missing playlist id\"}", "application/json");
        return;
    }

    CGetPlaylistInfoTask *task = new CGetPlaylistInfoTask(plugin, id);
    task->AddRef();

    HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

    if (SUCCEEDED(hr))
    {
        const auto &r = task->GetResult();
        if (r.found)
        {
            res.status = 200;
            res.set_content(json{
                                {"id", r.id},
                                {"name", r.name},
                                {"item_count", r.itemCount},
                                {"duration", r.duration},
                                {"playing_index", r.playingIndex},
                                {"is_read_only", r.isReadOnly}}
                                .dump(),
                            "application/json; charset=utf-8");
        }
        else
        {
            res.status = 404;
            res.set_content("{\"error\":\"Playlist not found\"}", "application/json");
        }
    }
    else
    {
        res.status = 500;
        res.set_content("{\"error\":\"ExecuteInMainThread Failed\"}", "application/json");
    }

    task->Release();
}

static void HandleGetPlaylistStats(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    std::string id = req.get_param_value("id");
    if (id.empty())
    {
        res.status = 400;
        res.set_content("{\"error\":\"Missing playlist id\"}", "application/json");
        return;
    }

    CGetPlaylistStatsTask *task = new CGetPlaylistStatsTask(plugin, id);
    task->AddRef();

    HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

    if (SUCCEEDED(hr))
    {
        const auto &r = task->GetResult();
        if (r.found)
        {
            res.status = 200;
            res.set_content(json{
                                {"genres", r.genres},
                                {"artists", r.artists},
                                {"artist_count", r.artistCount},
                                {"album_count", r.albumCount},
                                {"avg_bitrate", r.avgBitrate},
                                {"avg_rating", r.avgRating},
                                {"tracks_with_rating", r.tracksWithRating},
                                {"total_play_count", r.totalPlayCount},
                                {"tracks_never_played", r.tracksNeverPlayed},
                                {"total_size_bytes", r.totalSizeBytes}}
                                .dump(),
                            "application/json; charset=utf-8");
        }
        else
        {
            res.status = 404;
            res.set_content("{\"error\":\"Playlist not found\"}", "application/json");
        }
    }
    else
    {
        res.status = 500;
        res.set_content("{\"error\":\"ExecuteInMainThread Failed\"}", "application/json");
    }

    task->Release();
}

static void HandleGetPlaylistItems(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    std::string id = req.get_param_value("id");
    if (id.empty())
    {
        res.status = 400;
        res.set_content("{\"error\": \"Missing Playlist ID\"}", "application/json");
        return;
    }

    CGetPlaylistItemsTask *task = new CGetPlaylistItemsTask(plugin, id);
    task->AddRef();

    HRESULT hr = plugin->GetThreadService()->ExecuteInMainThread(task, AIMP_SERVICE_THREADS_FLAGS_WAITFOR);

    if (SUCCEEDED(hr))
    {
        json response = json::array();
        const auto &songs = task->GetResults();

        for (const auto &s : songs)
        {
            response.push_back({{"index", s.index},
                                {"title", s.title},
                                {"artist", s.artist},
                                {"album", s.album},
                                {"bitate", s.bitrate},
                                {"sampleRate", s.sampleRate},
                                {"duration", s.duration}});
        }
        res.set_content(response.dump(), "application/json; charset=utf-8");
    }
    else
    {
        res.status = 500;
    }

    task->Release();
}

static void HandlePlayPlaylistItem(MyPlugin *plugin, const httplib::Request &req, httplib::Response &res)
{
    if (!req.has_param("id") || !req.has_param("index"))
    {
        res.status = 400;
        res.set_content("{\"error\":\"Missing Playlist ID or Song index\"}", "application/json");
        return;
    }

    std::string plId = req.get_param_value("id");
    int index = std::stoi(req.get_param_value("index"));

    CPlayItemTask *task = new CPlayItemTask(plugin, plId, index);
    task->AddRef();

    if (SUCCEEDED(plugin->GetThreadService()->ExecuteInMainThread(task, 0)))
    {
        res.set_content("true", "application/json");
    }
    else
    {
        res.status = 500;
        res.set_content("{\"error\":\"Failed play\"}", "application/json");
    }

    task->Release();
}

// =============================================================================
// Route Registration
// =============================================================================

void RegisterPlaylistRoutes(MyPlugin* plugin)
{
    auto &svr = plugin->GetHttpServer();

    // GET endpoints
    svr.Get("/playlist/list", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetPlaylistList(plugin, req, res); });

    svr.Get("/playlist/current", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetCurrentPlaylist(plugin, req, res); });

    svr.Get("/playlist/info", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetPlaylistInfo(plugin, req, res); });

    svr.Get("/playlist/stats", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetPlaylistStats(plugin, req, res); });

    svr.Get("/playlist/items", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandleGetPlaylistItems(plugin, req, res); });

    svr.Get("/playlist/play", [plugin](const httplib::Request &req, httplib::Response &res)
            { HandlePlayPlaylistItem(plugin, req, res); });
}