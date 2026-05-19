# Fluke: AIMP Remote Control

A plugin for [AIMP](https://www.aimp.ru) that exposes a REST API and WebSocket server for remote control of the audio player. Designed for AIMP v5.40 (build 2709) and above.

## Features

- Full player control: play/pause, next, previous, seek, volume, mute, shuffle, repeat
- Real-time track change events via WebSocket (track metadata, position, player state)
- Track information and cover art retrieval
- Playlist browsing: list, info, statistics, items, and direct playback
- QR code display in AIMP settings dialog for easy connection from mobile devices
- No external client required -- works with any HTTP/WebSocket client

## Architecture

The plugin runs two servers inside the AIMP process:

| Server       | Port | Library        | Purpose                          |
|--------------|------|----------------|----------------------------------|
| HTTP (REST)  | 3553 | cpp-httplib    | Player control and data queries  |
| WebSocket    | 3554 | ixwebsocket    | Real-time event broadcasting     |

The plugin uses AIMP's Remote Access API (window messages `WM_AIMP_COMMAND` and `WM_AIMP_PROPERTY`) to interact with the player, and listens to AIMP core messages via `IAIMPMessageHook` for real-time events.

## API Endpoints

### Player

| Method | Path                   | Description                           |
|--------|------------------------|---------------------------------------|
| GET    | `/player/state`        | Get player state, position, volume    |
| GET    | `/player/volume`       | Get current volume (0-100)            |
| POST   | `/player/playpause`    | Toggle playback                       |
| POST   | `/player/next`         | Skip to next track                    |
| POST   | `/player/previous`     | Go to previous track                  |
| POST   | `/player/volume`       | Set volume (`{"volume": 0-100}`)      |
| POST   | `/player/seek`         | Seek to position (`{"position": ms}`) |
| POST   | `/player/mute`         | Toggle mute                           |
| POST   | `/player/shuffle`      | Toggle shuffle                        |
| POST   | `/player/repeat`       | Toggle repeat                         |

### Track

| Method | Path           | Description                                |
|--------|----------------|--------------------------------------------|
| GET    | `/track/info`  | Current track metadata (title, artist, etc) |
| GET    | `/track/cover` | Current track cover art image               |

### Playlist

| Method | Path                    | Description                        |
|--------|-------------------------|------------------------------------|
| GET    | `/playlist/list`        | List all playlists                 |
| GET    | `/playlist/current`     | Get current active playlist        |
| GET    | `/playlist/info`        | Get playlist details               |
| GET    | `/playlist/stats`       | Get playlist statistics            |
| GET    | `/playlist/items`       | Get all items in a playlist        |
| GET    | `/playlist/play`        | Play a specific item from playlist |

### WebSocket Events

The WebSocket server broadcasts JSON events in real time:

| Event               | Description                          |
|---------------------|--------------------------------------|
| `track_changed`     | A new track started playing          |
| `player_state`      | Player state changed (0/1/2)         |
| `position`          | Playback position update (every sec) |
| `volume_changed`    | Volume level changed                 |
| `mute_changed`      | Mute toggled                         |
| `shuffle_changed`   | Shuffle mode toggled                 |
| `repeat_changed`    | Repeat mode toggled                  |

## Build

### Prerequisites

- Visual Studio 2022 (v143 platform toolset)
- [vcpkg](https://vcpkg.io) with manifest mode (enabled by default)
- Windows SDK 10.0

### Dependencies

All managed via vcpkg manifest (`vcpkg.json`):

- [cpp-httplib](https://github.com/yhirose/cpp-httplib) -- HTTP server
- [nlohmann-json](https://github.com/nlohmann/json) -- JSON serialization
- [ixwebsocket](https://github.com/machinezone/IXWebSocket) -- WebSocket server

### Steps

1. Open the solution file `aimp_remote_reitansora.slnx` in Visual Studio.
2. Build the solution. vcpkg will automatically restore dependencies.
3. The output DLL (`aimp_remote_reitansora.dll`) will be placed in the build output directory.

### Installation

Copy the built DLL to AIMP's plugin directory (typically `%APPDATA%\AIMP\Plugins` or the AIMP installation `Plugins` folder). Enable the plugin in AIMP's plugin manager.

## Configuration

Server ports and bind addresses are compile-time constants defined in `src/core/Config.h`:

| Constant           | Default    | Description                   |
|--------------------|------------|-------------------------------|
| `HTTP_HOST`        | `0.0.0.0`  | HTTP server bind address      |
| `HTTP_PORT`        | `3553`     | HTTP server port              |
| `WEBSOCKET_HOST`   | `0.0.0.0`  | WebSocket server bind address |
| `WEBSOCKET_PORT`   | `3554`     | WebSocket server port         |

## Project Structure

```
src/
  core/           -- Plugin lifecycle, events, UI, utilities
  routes/         -- HTTP endpoint handlers (player, playlist, track)
  models/         -- Data transfer structs (Track, Playlist)
  tasks/          -- Async AIMP tasks for playlist operations
  helpers/        -- Network helper, QR code generation
  third_party/    -- Bundled QR Code generator library
sdk/
  aimp/5.40/      -- AIMP SDK headers
```

## Screenshots

<img width="453" alt="Plugin in the plugins list of AIMP" src="https://github.com/user-attachments/assets/3f42b6e9-f9b1-42ff-a980-461aa8f90224" />
<img width="744" alt="AIMP Remote options page with QR code and connection instructions" src="https://github.com/user-attachments/assets/b1a4234e-7da1-42ec-9f53-97715d2a0991" />

## License

Distributed under the MIT License. See [LICENSE](LICENSE) for more information.

## Author

Stiven Pilca (ReitanSora)
