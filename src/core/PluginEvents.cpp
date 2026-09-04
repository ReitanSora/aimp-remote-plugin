#include "pch.h"
#include "Plugin.h"
#include "../helpers/TrackInfoHelper.h"
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

        json info = GetTrackInfo(this);

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
