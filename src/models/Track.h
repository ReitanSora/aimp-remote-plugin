#pragma once

#include <string>

/**
 * @file Track.h
 * @brief Defines the SongData struct used to hold track metadata.
 *
 * `SongData` is a lightweight POD-like container for common audio track
 * metadata used across the application (e.g., UI display, remote control,
 * logging). All string fields use `std::string` (UTF-8 compatible on most
 * platforms). Numeric fields use conventional units described below.
 */

 /**
  * @brief Metadata for a single audio track.
  *
  * Notes on fields:
  * - `index` : Track index or playlist position. Stored as a string because
  *   some sources provide padded numbers or non-numeric prefixes (e.g., "01",
  *   "A-1").
  * - `title` : Track title.
  * - `artist`: Performing artist(s).
  * - `album` : Album name.
  * - `bitrate`: Bitrate in kilobits per second (kbps). Use 0 when unknown.
  * - `sampleRate`: Sample rate in Hertz (Hz). Use 0 when unknown.
  * - `duration`: Duration in seconds (floating-point). Use 0.0 when unknown.
  */
struct SongData
{
    std::string index;     ///< Track index or playlist position (string to preserve formatting)
    std::string title;     ///< Track title
    std::string artist;    ///< Performing artist(s)
    std::string album;     ///< Album name
    int bitrate;           ///< Bitrate in kbps (0 if unknown)
    int sampleRate;        ///< Sample rate in Hz (0 if unknown)
    double duration;       ///< Duration in seconds (0.0 if unknown)
};