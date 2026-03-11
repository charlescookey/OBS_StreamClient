# OBS Stream Client

`OBS Stream Client` is a small Qt Widgets desktop app for controlling one or more OBS Studio instances over the OBS WebSocket protocol.

The current project is a focused prototype: it connects to named OBS servers, authenticates with OBS WebSocket v5, and sends a few broadcast control commands from a simple GUI.

## What It Does

- Connects to OBS over WebSocket using `QWebSocket`
- Performs OBS WebSocket authentication
- Keeps a small in-memory map of named OBS clients
- Sends commands to all connected clients at once
- Provides quick buttons for:
  - connecting to clint 1 OBS instance
  - connecting to clint 2 OBS instance
  - switching all connected clients to the selected scene
  - starting the stream on all connected clients

## Tech Stack

- C++
- Qt Widgets
- Qt WebSockets
- qmake

## Project Structure

- [`main.cpp`](/home/aru/side_work/OBS_StreamClient/main.cpp): Qt application entry point
- [`mainwindow.h`](/home/aru/side_work/OBS_StreamClient/mainwindow.h): main window declarations and client registry
- [`mainwindow.cpp`](/home/aru/side_work/OBS_StreamClient/mainwindow.cpp): button handlers and OBS control flow
- [`mainwindow.ui`](/home/aru/side_work/OBS_StreamClient/mainwindow.ui): Qt Designer UI layout
- [`obsclient.h`](/home/aru/side_work/OBS_StreamClient/obsclient.h): OBS client interface
- [`obsclient.cpp`](/home/aru/side_work/OBS_StreamClient/obsclient.cpp): WebSocket connection, auth handshake, and request sending

## Requirements

- Qt with:
  - `core`
  - `gui`
  - `widgets`
  - `websockets`
- A C++17-compatible compiler
- OBS Studio with OBS WebSocket enabled on the target machines

## Build

### Command Line

```bash
qmake TestOBS.pro
make
./TestOBS
```

On some systems you may need `qmake6` instead of `qmake`.

## How It Works

When a connection is opened, OBS sends a `Hello` message. The app:

1. Reads the authentication challenge from OBS.
2. Computes the auth response using SHA-256 and Base64.
3. Sends an `Identify` message with `rpcVersion: 1`.
4. Waits for OBS to confirm authentication.
5. Sends requests such as `SetCurrentProgramScene` or `StartStream`.

The low-level protocol handling lives in [`obsclient.cpp`](/home/aru/side_work/OBS_StreamClient/obsclient.cpp).

## Suggested Next Steps

- add a settings screen for OBS hosts, ports, passwords, and scene names
- show per-client connection/authentication status
- support stop stream, start recording, stop recording, and scene collection controls
- replace raw pointers with managed ownership
- rename the UI controls to match their actual actions
