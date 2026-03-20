# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

NexPad is a Windows C++ console application that maps Xbox/PlayStation controller input to keyboard and mouse events, enabling controller-based PC navigation. It targets Windows 10/11 and uses XInput for Xbox controllers with a native USB HID fallback for PS5 DualSense controllers.

## Build

The project uses Visual Studio (MSVC v143 toolset, Windows 10 SDK). Build via Visual Studio's solution file:

- **Solution:** `NexPad.sln`
- **Project:** `NexPad/NexPad.vcxproj`
- **Output binary name:** `NexPad.exe`
- **Configurations:** `Debug|Win32` and `Release|Win32`
- **Release output:** `Release/NexPad.exe` (alongside `Release/config.ini`)

To build from the command line using MSBuild:
```
msbuild NexPad.sln /p:Configuration=Release /p:Platform=Win32
```

There are no automated tests.

## Architecture

The program is structured around three main classes:

### `CXBOXController` (`CXBOXController.h/cpp`)
Low-level controller input layer. On `GetState()`, it first tries XInput (dynamically loaded from `xinput1_4.dll`, `xinput1_3.dll`, or `xinput9_1_0.dll` in that order). If XInput fails, it falls back to raw USB HID polling of a Sony DualSense (VID `0x054C`, PID `0x0CE6` or `0x0DF2`). The DualSense HID report is parsed and mapped into an `XINPUT_STATE` struct so the rest of the code is controller-agnostic.

### `NexPad` (`NexPad.h/NexPad.cpp`)
Core logic class. Owns a pointer to a `CXBOXController`. Responsibilities:
- **`loadConfigFile()`** — reads `config.ini` from the working directory using `ConfigFile`, populates all `CONFIG_*` and `GAMEPAD_*` member variables (as `DWORD` virtual key / XInput button bitmask values), and the speed list.
- **`loop()`** — called in an infinite loop from `main()` at ~150 Hz (`SLEEP_AMOUNT = 1000/FPS`). Each iteration: reads controller state, checks disable toggle, then dispatches to `handleMouseMovement()`, `handleScrolling()`, mouse click mappers, OSK toggle, speed change, and all digital button-to-key mappings.
- **Button state tracking** — `setXboxClickState(DWORD STATE)` uses `std::map` keyed by XInput button bitmask to track `_xboxClickIsDown` (rising edge), `_xboxClickIsUp` (falling edge), and `_xboxClickIsDownLong` (held > 200 ms). Button combos work by OR-ing bitmasks (e.g., `0x0030` = START + BACK).
- **`_pressedKeys`** list tracks all currently-pressed keys/mouse buttons so they can all be released cleanly when NexPad is disabled.
- **`ON_ENABLE` / `ON_DISABLE`** keys are sent as a keydown+keyup pair when toggling the disable state, allowing external software (e.g., game overlays) to react to NexPad's enabled/disabled transitions.

### `ConfigFile` (`ConfigFile.h/ConfigFile.cpp`) + `Convert` (`Convert.h`)
INI-style config parser. Strips `#` comments, parses `key = value` lines into a `std::map<string,string>`. Values are retrieved via the templated `getValueOfKey<T>()`, which delegates type conversion to `Convert::string_to_T<T>`. Config is read from `config.ini` in the **current working directory** (not the executable's directory).

### `main.cpp`
Entry point. Instantiates `CXBOXController(1)` (player 1) and a `NexPad` instance, calls `loadConfigFile()`, then runs the application loop. Also contains an unused `ChangeVolume()` helper and an `isRunningAsAdministrator()` check (admin rights are needed to interact with the on-screen keyboard).

## Config File

`Release/config.ini` is the shipped default. If missing, NexPad will fail to parse. Config values use:
- XInput button bitmask hex values (see MSDN `XINPUT_GAMEPAD`) for `CONFIG_*` and `GAMEPAD_*` keys.
- Virtual key codes (see MSDN Virtual-Key Codes) for keyboard mappings.
- Combo buttons: sum the hex values (e.g., `0x0010 + 0x0020 = 0x0030` triggers on START+BACK simultaneously).
- `CURSOR_SPEED` is a comma-separated list of `NAME=value` pairs; all speeds in range `(0.0001, 1.0]` are accepted.
- `INIT_DISABLED = 1` starts NexPad in disabled mode (useful when launched at startup for toggling via `ON_ENABLE`/`ON_DISABLE`).
