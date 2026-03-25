# NexPad Automated Test Harness Design

**Date:** 2026-03-25
**Issue:** #8 — Add automated controller regression test harness
**Milestone:** Longer-term

## Goal

Introduce a Google Test regression harness that covers `ConfigFile` parsing, DualSense HID report parsing, and `NexPad` button state logic — all runnable without real hardware — and wire it into CI so regressions are caught before release.

## Approach

Google Test via git submodule (`googletest/`), built from source. A new VS project (`NexPadTests`) in the existing solution compiles the logic source files directly. One small extraction from `CXBOXController` and two minimal accessor additions to `NexPad` make the untestable code testable.

## New Files and Changes

### New: `googletest/` submodule

Git submodule at repo root pointing to `https://github.com/google/googletest`. The test project builds `googletest/googletest/src/gtest-all.cc` directly — no external install required.

### New: `Windows/NexPadTests/NexPadTests.vcxproj`

New VS project added to `Windows/NexPad.sln`. Configuration:
- Output type: Application (Console)
- Compiles: `CXBOXController.cpp`, `ConfigFile.cpp`, `NexPad.cpp`, `RuntimeLoop.cpp`, `gtest-all.cc`, `test_*.cpp`
- Excludes: `main.cpp` (Win32 UI entry point — not testable)
- Include paths: `Windows/NexPad/`, `googletest/googletest/include/`, `googletest/googletest/`
- Preprocessor: `GTEST_HAS_SEH=0`
- Links: standard Win32 libs (`xinput.lib`, `hid.lib`)

### New: `Windows/NexPadTests/test_configfile.cpp`

Tests for `ConfigFile` and `Convert`:
- `key = value` parsing
- `#` comment stripping
- Missing key returns default value
- `getValueOfKey<int>`, `getValueOfKey<float>`, `getValueOfKey<std::string>`

### New: `Windows/NexPadTests/test_controller_parsing.cpp`

Tests for the extracted `ParseDualSenseReport()` free function:
- Known USB HID byte buffer → correct `XINPUT_STATE` button bitmask
- Known thumbstick bytes → correct axis values in expected range
- Known touchpad bytes → correct `TouchpadState` coords
- Short/empty buffer → returns false, output zeroed

### New: `Windows/NexPadTests/test_button_state.cpp`

Tests for `NexPad` button edge detection (calls `setXboxClickState()` + new accessors):
- Single button: `isButtonDown()` true only on the tick the button is first pressed
- Single button: `isButtonUp()` true only on the tick the button is released
- Hold >200ms: `isButtonDownLong()` becomes true
- Hold then release: `isButtonDown()` false after first tick, `isButtonUp()` true on release
- Combo (e.g., `0x0030` = START+BACK): fires when both bits set simultaneously

### Modified: `Windows/NexPad/CXBOXController.h` + `CXBOXController.cpp`

Extract DualSense HID report parsing into a free function:

```cpp
// CXBOXController.h
bool ParseDualSenseReport(const uint8_t* buf, size_t len,
                          XINPUT_GAMEPAD& gamepadOut,
                          CXBOXController::TouchpadState& touchpadOut);
```

The function is currently inline logic inside `GetState()`. Extracting it does not change `GetState()` behavior — `GetState()` calls `ParseDualSenseReport()` after the extraction.

### Modified: `Windows/NexPad/NexPad.h` + `NexPad.cpp`

Add three minimal test accessors and make `setXboxClickState()` public:

```cpp
// Make public (move from private section):
void setXboxClickState(DWORD state);

// New accessors:
bool isButtonDown(DWORD key) const;      // returns _xboxClickIsDown[key]
bool isButtonUp(DWORD key) const;        // returns _xboxClickIsUp[key]
bool isButtonDownLong(DWORD key) const;  // returns _xboxClickIsDownLong[key]
```

These are the minimum changes to make the button state logic testable without calling `processCurrentState()` (which triggers `SendInput` and other Win32 side effects).

### Modified: `.github/workflows/build.yml`

Add a test step after the Release builds:

```yaml
- name: Run NexPadTests
  run: |
    .\release\x64\NexPadTests.exe --gtest_output=xml:test-results.xml
```

Non-zero exit fails the workflow. Test results uploaded as a workflow artifact for inspection.

## What Is Not Tested

- Mouse movement, scrolling, `SendInput` dispatch — these require the Win32 subsystem
- XInput dynamic DLL loading — hardware-dependent
- OSK launch, vibration — hardware/UI-dependent
- `main.cpp` Win32 dialog UI — excluded from the test project

These remain under manual hardware validation per `docs/controller-compatibility.md`.

## Done When

- `NexPadTests.exe` runs and passes in CI on every push to `main`
- At least one test each for `ConfigFile`, `ParseDualSenseReport`, and button edge detection
- Future controller changes can be validated without exclusive reliance on manual retesting
