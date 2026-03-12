# OBS Stream Client

`OBS Stream Client` is a Qt Widgets desktop app for controlling multiple OBS Studio instances over the OBS WebSocket protocol.

The current app supports up to 20 saved clients, a dedicated status view, single-client editing in the settings tab, and a dropdown-driven control panel for the currently selected OBS target.

## Features

- Up to 20 configurable OBS client profiles
- Local settings storage with `QSettings`
- Three-tab layout:
  - `Status`
  - `Settings`
  - `Controls`
- Status table for all clients with connection state
- Red/green status coloring for disconnected vs connected clients
- Single-profile editing flow with `New Client`
- Single-client control panel selected from a dropdown
- Broadcast actions for all saved clients
- OBS WebSocket authentication and request handling

## UI Overview

### Status Tab

- Lists all saved clients
- Shows endpoint and connection state
- Uses green for connected clients and red for disconnected clients
- Includes `Connect Selected` and `Disconnect Selected`

### Settings Tab

- Edits one client at a time
- Lets you switch between clients with a selector
- Adds new client profiles with `New Client`
- Stores name, host, port, and password
- Stores default scene and scene collection values
- Lets you load available scene collections from the currently connected selected client with `Load From OBS`

### Controls Tab

- Keeps broadcast controls for all clients
- Uses a dropdown to pick one active client
- Shows the selected client's name, endpoint, and state
- Provides actions for the selected client:
  - connect
  - disconnect
  - switch scene
  - apply scene collection
  - start stream
  - stop stream
  - start recording
  - stop recording

## Requirements

- Qt 6 with:
  - `core`
  - `gui`
  - `widgets`
  - `websockets`
- A C++17-compatible compiler
- OBS Studio with WebSocket enabled on the target machines

## Build

```bash
qmake6 TestOBS.pro
make
./TestOBS
```

The current code was verified with `qmake6` and `make`.

## Project Structure

- [`main.cpp`](/home/aru/side_work/OBS_StreamClient/main.cpp): Qt entry point
- [`mainwindow.h`](/home/aru/side_work/OBS_StreamClient/mainwindow.h): main window declarations and client model
- [`mainwindow.cpp`](/home/aru/side_work/OBS_StreamClient/mainwindow.cpp): UI wiring, settings persistence, client status updates, and OBS actions
- [`mainwindow.ui`](/home/aru/side_work/OBS_StreamClient/mainwindow.ui): three-tab Qt Designer layout
- [`obsclient.h`](/home/aru/side_work/OBS_StreamClient/obsclient.h): OBS WebSocket client API
- [`obsclient.cpp`](/home/aru/side_work/OBS_StreamClient/obsclient.cpp): connection, authentication, request, and error handling
