#pragma once

class MyPlugin;
/**
 * @brief Registers all playlist-related HTTP endpoints
 * @param plugin Pointer to MyPlugin instance
 *
 * Endpoints registered:
 * - GET  /playlist/list       - Get list of playlists
 * - GET  /playlist/current    - Get current playlist
 * - GET  /playlist/info       - Get info about a playlist
 * - GET  /playlist/stats      - Get stats about a playlist
 * - GET  /playlist/items      - Get all items of a playlist
 * - GET  /playlist/play       - Play a selected item of a playlist
 */
void RegisterPlaylistRoutes(MyPlugin *plugin);