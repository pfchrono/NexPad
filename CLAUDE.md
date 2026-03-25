# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

NexPad is a native Windows C++ utility (no runtime-heavy frontend) that maps Xbox/PlayStation controller input to mouse and keyboard events for couch-first desktop navigation. Targets Windows 10/11 via MSVC v143 toolset.

## Build

**Visual Studio:** Open `Windows/NexPad.sln`

**MSBuild CLI:**
```powershell
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=Win32
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64
```

**Build all configurations (Debug/Release × Win32/x64):**
```powershell
.\scripts\build-all.ps1          # PowerShell
.\scripts\build-all.bat          # Command Prompt
.\scripts\build-all.ps1 -Clean   # Clean before build
```

**Release packaging:**
```powershell
.\scripts\package-release.ps1 -Version vX.Y.Z
```
Produces zipped artifacts + SHA256 checksums under `artifacts/`.

**Build output:** `debug/x32/`, `debug/x64/`, `release/x32/`, `release/x64/` — each containing `NexPad.exe`, `config.ini`, and `presets/`. Intermediate files go to `Windows/.build/`.

There are no automated tests. Validation is manual hardware testing against controllers.

## Architecture

All source lives under `Windows/NexPad/`. Three core classes plus the entry point:

### `CXBOXController` (`CXBOXController.h/cpp`)
Low-level input layer. `GetState()` tries XInput first (dynamically loads `xinput1_4.dll` → `xinput1_3.dll` → `xinput9_1_0.dll`). On failure, falls back to raw USB HID polling for Sony DualSense (VID `0x054C`, PID `0x0CE6` or `0x0DF2`). DualSense HID report is parsed into `XINPUT_STATE` so all downstream code is controller-agnostic.

### `NexPad` (`NexPad.h/NexPad.cpp`)
Core logic. Owns a `CXBOXController` pointer.
- **`loadConfigFile()`** — reads `config.ini` from the working directory, populates `CONFIG_*`/`GAMEPAD_*` members as `DWORD` XInput bitmasks and VK codes.
- **`loop()`** — runs at ~150 Hz (`SLEEP_AMOUNT = 1000/FPS`). Reads controller state, checks disable toggle, dispatches to mouse movement, scrolling, click mappers, OSK toggle, speed cycling, and all button-to-key bindings.
- **Button state** — `setXboxClickState()` tracks `_xboxClickIsDown` (rising edge), `_xboxClickIsUp` (falling edge), `_xboxClickIsDownLong` (held >200 ms) per XInput bitmask key. Combos = OR'd bitmasks (e.g., `0x0030` = START+BACK).
- **`_pressedKeys`** — list of currently held keys/buttons; all released cleanly on disable.
- **`ON_ENABLE`/`ON_DISABLE`** — keydown+keyup pair sent on toggle, allowing external apps to react.

### `ConfigFile` (`ConfigFile.h/cpp`) + `Convert` (`Convert.h`)
INI parser. Strips `#` comments, parses `key = value` into `std::map<string,string>`. Values retrieved via templated `getValueOfKey<T>()` with type conversion through `Convert::string_to_T<T>`. Reads from the **current working directory**, not the executable's directory.

### `main.cpp`
Entry point **and** the entire Win32 UI. Contains the native dialog with three tabs (Status, Settings, Mappings), preset save/load/import/export, theme support (dark/light/high-contrast via `UI_THEME_MODE`), and an `isRunningAsAdministrator()` check (needed for on-screen keyboard interaction).

## Config File

`config.ini` (repo root, also copied next to the executable at build time) must be present next to the executable at runtime. Key groups:
- `CONFIG_*` — controller actions (mouse clicks, disable toggle, OSK)
- `GAMEPAD_*` — button-to-key mappings (XInput bitmask hex → VK code)
- `ON_ENABLE` / `ON_DISABLE` — optional VK events fired on toggle
- `CURSOR_SPEED` — comma-separated `NAME=value` list; values in `(0.0001, 1.0]`
- `TOUCHPAD_ENABLED`, `TOUCHPAD_SPEED`, `TOUCHPAD_DEAD_ZONE` — DualSense one-finger cursor
- `TOUCHPAD_SCROLL_ENABLED`, `TOUCHPAD_SCROLL_SPEED` — DualSense two-finger scroll (independent)
- `INIT_DISABLED = 1` — start in disabled state (useful for startup launch + `ON_ENABLE` flow)
- `START_WITH_WINDOWS` — registers run key for current user
- `UI_THEME_MODE` — `0` dark, `1` light, `2` high contrast

Reference: [Virtual-Key Codes](https://msdn.microsoft.com/en-us/library/windows/desktop/dd375731), [XInput button values](https://msdn.microsoft.com/en-us/library/windows/desktop/microsoft.directx_sdk.reference.xinput_gamepad%28v=vs.85%29.aspx)

## Release Publishing

CI is defined in `.github/workflows/build.yml`. For a tag build:
```powershell
git tag vX.Y.Z
git push origin vX.Y.Z
```
The workflow builds, packages, and attaches artifacts to the GitHub Release automatically. See `RELEASE_CHECKLIST.md` for the manual process.

## Feature Development Workflow

This repo uses a spec-driven workflow via `.specify/`, `.github/prompts/`, and `.github/agents/`:

1. `/speckit.specify` — write the feature spec (user-visible behavior)
2. `/speckit.plan` — produce implementation plan with NexPad file paths and build validation steps
3. `/speckit.tasks` — break plan into actionable tasks
4. `/speckit.implement` — execute approved tasks

Plans for NexPad changes should always cover: affected files under `Windows/NexPad/`, MSBuild validation, manual controller hardware checks when input behavior changes, and config/README updates when runtime behavior changes.
