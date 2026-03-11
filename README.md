# OBS Stream Client

`OBS Stream Client` is a Qt Widgets desktop app for controlling up to two OBS Studio instances over the OBS WebSocket protocol.

The app now includes a proper settings screen, a separate controls tab, saved connection profiles, per-client status, and broadcast actions for common stream operations.

## Features

- Two configurable OBS client profiles
- Saved host, port, password, scene, and scene collection settings via `QSettings`
- Separate `Settings` and `Controls` tabs
- Per-client connection status and activity log
- Per-client actions:
  - connect
  - disconnect
  - switch program scene
  - apply scene collection
  - start stream
  - stop stream
  - start recording
  - stop recording
- Broadcast actions that run the same command on both configured clients
- OBS WebSocket authentication handling for OBS v5-style handshake

## UI Overview

### Settings Tab

Use the `Settings` tab to configure:

- Client 1 name, host, port, and password
- Client 2 name, host, port, and password
- Default program scene name
- Default scene collection name

Press `Save Settings` to persist the configuration locally.

### Controls Tab

Use the `Controls` tab for live operation:

- `Broadcast Controls` for connect/disconnect, scene switching, scene collection changes, streaming, and recording across all configured clients
- Individual client panels for one-off actions on a single OBS instance
- A live activity log showing connection, authentication, and request results

## Requirements

- Qt 6 with:
  - `core`
  - `gui`
  - `widgets`
  - `websockets`
- A C++17-compatible compiler
- OBS Studio with WebSocket enabled on the target machines

## Build

### Command Line

```bash
qmake6 TestOBS.pro
make
./TestOBS
```

If your system exposes a configured `qmake`, that should work too. In this environment the verified build command was `qmake6 TestOBS.pro`.

### Qt Creator

1. Open [`TestOBS.pro`](/home/aru/side_work/OBS_StreamClient/TestOBS.pro).
2. Select a Qt kit with the WebSockets module installed.
3. Build and run the project.

## Project Structure

- [`main.cpp`](/home/aru/side_work/OBS_StreamClient/main.cpp): Qt application entry point
- [`mainwindow.h`](/home/aru/side_work/OBS_StreamClient/mainwindow.h): main window declarations and app state
- [`mainwindow.cpp`](/home/aru/side_work/OBS_StreamClient/mainwindow.cpp): UI wiring, settings persistence, status updates, and OBS control actions
- [`mainwindow.ui`](/home/aru/side_work/OBS_StreamClient/mainwindow.ui): tabbed Qt Designer UI
- [`obsclient.h`](/home/aru/side_work/OBS_StreamClient/obsclient.h): OBS WebSocket client API
- [`obsclient.cpp`](/home/aru/side_work/OBS_StreamClient/obsclient.cpp): authentication, request dispatch, and response/error handling

## How It Works

When the app connects to OBS:

1. A WebSocket connection is opened.
2. OBS sends its `Hello` message.
3. The app computes the authentication response if OBS requires it.
4. The app sends `Identify`.
5. After authentication succeeds, UI actions send OBS requests such as:
   - `SetCurrentProgramScene`
   - `SetCurrentSceneCollection`
   - `StartStream`
   - `StopStream`
   - `StartRecord`
   - `StopRecord`

## Notes

- Settings are stored locally using `QSettings`.
- Passwords are no longer hardcoded in source, but they are still stored locally in app settings.
- The app currently supports two OBS targets. If you need more, the current structure is ready to be generalized further.

## Future Improvements

- Add scene and scene collection discovery directly from OBS instead of manual text entry
- Add visual badges or colored indicators for connection and auth state
- Add support for stream/record state polling
- Expand from two fixed clients to a dynamic client list
- Offer secure secret storage instead of plain local settings
