#pragma once

#include <string>
#include <vector>
#include <basetsd.h>

/// @file Playlist.h
/// @brief Lightweight POD structures used to represent playlist-related data
/// in the application.
///
/// These structs are intentionally simple and used for data transfer between
/// components (serialization, RPC, UI view models, etc.). Members use
/// sensible defaults where applicable.

/// Basic summary information for a playlist.
/// Typically used when listing playlists.
struct PlaylistData
{
    /// Unique identifier for the playlist (implementation-specific).
    std::string id;

    /// Human-readable playlist name.
    std::string name;

    /// Number of items (tracks) contained in the playlist.
    int itemCount;
};

/// Information about the current (active) playlist.
/// Extends the basic summary with a flag that indicates if the playlist
/// was successfully resolved/found.
struct CurrentPlaylistData
{
    /// Unique identifier for the playlist (implementation-specific).
    std::string id;

    /// Human-readable playlist name.
    std::string name;

    /// Number of items (tracks) contained in the playlist.
    int itemCount;

    /// True when the current playlist was found/resolved successfully.
    /// Defaults to false.
    bool found = false;
};

/// Fast-access playlist information intended for operations that need
/// quick aggregate values (duration, playing index, readonly flag).
struct PlaylistInfoFastData
{
    /// Unique identifier for the playlist (implementation-specific).
    std::string id;

    /// Human-readable playlist name.
    std::string name;

    /// Number of items (tracks) contained in the playlist.
    /// Defaults to 0.
    int itemCount = 0;

    /// Total duration of the playlist in seconds.
    /// Defaults to 0.0.
    double duration = 0.0;

    /// Index of the currently playing item, or -1 if none.
    int playingIndex = -1;

    /// True when the playlist is read-only.
    bool isReadOnly = false;

    /// True when the playlist metadata was found/resolved successfully.
    bool found = false;
};

/// Aggregated statistics for a playlist.
/// Contains collections and computed metrics useful for analytics or UI.
struct PlaylistStatsData
{
    /// Unique genres present in the playlist.
    std::vector<std::string> genres;

    /// Unique artists present in the playlist.
    std::vector<std::string> artists;

    /// Number of distinct artists.
    int artistCount = 0;

    /// Number of distinct albums.
    int albumCount = 0;

    /// Average bitrate (kbps) across tracks. 0.0 if unknown.
    double avgBitrate = 0.0;

    /// Average user rating across rated tracks. 0.0 if none rated.
    double avgRating = 0.0;

    /// Sum of play counts for all tracks.
    int totalPlayCount = 0;

    /// Number of tracks that have an explicit rating.
    int tracksWithRating = 0;

    /// Number of tracks that have never been played.
    int tracksNeverPlayed = 0;

    /// Total size of all tracks in bytes.
    INT64 totalSizeBytes = 0;

    /// True when the statistics were computed / found successfully.
    bool found = false;
};