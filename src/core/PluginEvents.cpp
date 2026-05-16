#include "pch.h"

#include "Plugin.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// =========================================================================
// CoreMessage
// Called by AIMP on the main thread for every internal event.
// We filter the events we care about and broadcast JSON to WS clients.
// =========================================================================
void WINAPI MyPlugin::CoreMessage(LongWord AMessage, int AParam1, void* AParam2, HRESULT* AResult)
{
    switch (AMessage)
    {

        // ---------------------------------------------------------------------
        // New track started (or internet radio changed sub-track).
        // Read the current file info from IAIMPServicePlayer and broadcast it.
        // ---------------------------------------------------------------------
    case AIMP_MSG_EVENT_STREAM_START:
    case AIMP_MSG_EVENT_STREAM_START_SUBTRACK:
    {
        json info = BuildTrackInfoJson();
        if (!info.empty())
        {
            info["event"] = "track_changed";
            BroadcastWS(info);
        }
        break;
    }

    // ---------------------------------------------------------------------
    // Player state changed: stopped (0), paused (1), playing (2).
    // AParam1 carries the new state directly.
    // ---------------------------------------------------------------------
    case AIMP_MSG_EVENT_PLAYER_STATE:
    {
        BroadcastWS({ {"event", "player_state"},
                     {"state", AParam1} });
        break;
    }

    // ---------------------------------------------------------------------
    // Position timer tick — fires every second while playing.
    // AParam1/AParam2 are unused; we query the real position ourselves.
    // ---------------------------------------------------------------------
    case AIMP_MSG_EVENT_PLAYER_UPDATE_POSITION:
    {
        if (_playerService)
        {
            HWND hAIMP = FindWindowA(AIMPRemoteAccessClass, NULL);
            if (hAIMP)
            {
                LRESULT position = SendMessage(hAIMP, WM_AIMP_PROPERTY, AIMP_RA_PROPERTY_PLAYER_POSITION, 0);
                BroadcastWS({ {"event", "position"},
                             {"position", position} });
            }
        }
        break;
    }

    // ---------------------------------------------------------------------
    // A property changed.
    // AParam1 = property ID (AIMP_MSG_PROPERTY_XXX)
    // AParam2 = pointer to the new value (type depends on the property)
    // ---------------------------------------------------------------------
    case AIMP_MSG_EVENT_PROPERTY_VALUE:
    {
        switch (AParam1)
        {

            // User seeked to a new position manually.
            // AParam2 → float* (seconds)
        case AIMP_MSG_PROPERTY_PLAYER_POSITION:
        {
            float pos = *static_cast<float*>(AParam2);
            BroadcastWS({ {"event", "position"},
                         {"position", static_cast<double>(pos)},
                         {"seeked", true} });
            break;
        }

        // Mute toggled.
        // AParam2 → DWORD* (LongBool: 0 = unmuted, non-zero = muted)
        case AIMP_MSG_PROPERTY_MUTE:
        {
            bool muted = (*static_cast<DWORD*>(AParam2)) != 0;
            BroadcastWS({ {"event", "mute_changed"},
                         {"mute", muted} });
            break;
        }

        // Volume changed.
        // AParam2 → float* in [0.0 .. 1.0]; convert to 0..100.
        case AIMP_MSG_PROPERTY_VOLUME:
        {
            float vol = *static_cast<float*>(AParam2);
            BroadcastWS({ {"event", "volume_changed"},
                         {"volume", static_cast<int>(vol * 100.0f + 0.5f)} });
            break;
        }

        // Shuffle toggled.
        // AParam2 → DWORD* (LongBool)
        case AIMP_MSG_PROPERTY_SHUFFLE:
        {
            bool on = (*static_cast<DWORD*>(AParam2)) != 0;
            BroadcastWS({ {"event", "shuffle_changed"},
                         {"shuffle", on} });
            break;
        }

        // Repeat toggled.
        // AParam2 → DWORD* (LongBool)
        case AIMP_MSG_PROPERTY_REPEAT:
        {
            bool on = (*static_cast<DWORD*>(AParam2)) != 0;
            BroadcastWS({ {"event", "repeat_changed"},
                         {"repeat", on} });
            break;
        }

        } // inner switch (AParam1)
        break;
    }

    } // outer switch (AMessage)
}

// -------------------------------------------------------------------------
/// @brief BuildTrackInfoJson
///
/// Reads metadata for the currently playing track from IAIMP's player
/// service and returns it as a JSON object.
///
/// Notes:
/// - MUST be called from AIMP's main thread (for example, inside CoreMessage).
///   The IAIMP COM services used here are not thread-safe and can fail if
///   invoked from worker threads.
/// - If the player service is not available or there is no currently loaded
///   track, the function returns an empty JSON object.
///
/// Behavior & resource management:
/// - Queries string properties using `GetPropertyText` and playlist helpers,
///   and numeric properties using `GetValueAsFloat` / `GetValueAsInt32`.
/// - All COM objects acquired (`IAIMPFileInfo`, `IAIMPPlaylistItem`,
///   `IAIMPPlaylist`, `IAIMPPropertyList`, etc.) are properly `Release()`d
///   before the function returns to avoid leaks.
///
/// Returned JSON schema (keys are always present if function succeeds):
/// {
///   "title": string,        // track title or "Unknown"
///   "artist": string,       // track artist or "Unknown"
///   "album": string,        // album name or "Unknown"
///   "genre": string,        // genre or "Unknown"
///   "duration": number,     // duration in seconds (double)
///   "play_count": integer,  // play count (int)
///   "playlist_id": string,  // ID of the playlist containing the track (may be "")
///   "bitrate": integer,     // bitrate in kbps (int)
///   "sample_rate": integer, // sample rate in Hz (int)
///   "rating": integer       // user rating / ML mark (int)
/// }
///
/// Example:
/// {"title":"Track Name","artist":"Artist","album":"Album","duration":215.0, ...}
///
/// @return nlohmann::json JSON object with track metadata or an empty object on failure.
// -------------------------------------------------------------------------
json MyPlugin::BuildTrackInfoJson()
{
    if (!_playerService)
        return json::object();

    IAIMPFileInfo* fileInfo = nullptr;
    if (FAILED(_playerService->GetInfo(&fileInfo)))
        return json::object();

    std::string title = GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_TITLE, "Unknown");
    std::string artist = GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_ARTIST, "Unknown");
    std::string album = GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_ALBUM, "Unknown");
    std::string genre = GetPropertyText(fileInfo, AIMP_FILEINFO_PROPID_GENRE, "Unknown");

    double duration = 0.0;
    int bitrate = 0;
    int sampleRate = 0;
    int playCount = 0;
    int rating = 0;
    fileInfo->GetValueAsFloat(AIMP_FILEINFO_PROPID_DURATION, &duration);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_BITRATE, &bitrate);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_SAMPLERATE, &sampleRate);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_ML_MARK, &rating);
    fileInfo->GetValueAsInt32(AIMP_FILEINFO_PROPID_ML_PLAYCOUNT, &playCount);
    fileInfo->Release();

    // Read the playlist the current track belongs to.
    // GetPlaylistItem() only succeeds while a track is loaded.
    std::string playlistId = "";

    IAIMPPlaylistItem* plItem = nullptr;
    if (SUCCEEDED(_playerService->GetPlaylistItem(&plItem)))
    {
        IAIMPPlaylist* playlist = nullptr;
        if (SUCCEEDED(plItem->GetValueAsObject(AIMP_PLAYLISTITEM_PROPID_PLAYLIST,
            IID_IAIMPPlaylist, (void**)&playlist)))
        {
            IAIMPPropertyList* propList = nullptr;
            if (SUCCEEDED(playlist->QueryInterface(IID_IAIMPPropertyList, (void**)&propList)))
            {
                playlistId = GetPropertyTextPlaylist(propList, AIMP_PLAYLIST_PROPID_ID, "");
                propList->Release();
            }
            playlist->Release();
        }
        plItem->Release();
    }

    return {
        {"title", title},
        {"artist", artist},
        {"album", album},
        {"genre", genre},
        {"duration", duration},
        {"play_count", playCount},
        {"playlist_id", playlistId},
        {"bitrate", bitrate},
        {"sample_rate", sampleRate},
        {"rating", rating} };
}