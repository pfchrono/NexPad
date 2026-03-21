# Quickstart: DualSense Two-Finger Scrolling

## Purpose

Use this guide after implementation to validate the feature against the real NexPad build and runtime behavior.

## Prerequisites

- Windows 10 or 11 with Visual Studio 2022 build tools available.
- A DualSense controller that can be tested over USB and Bluetooth.
- A scrollable Windows target such as File Explorer, Notepad, a browser, or Settings.
- Repository root at `F:/code/NexPad`.

## Build

Run from the repository root:

```powershell
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe' .\Windows\NexPad.sln /p:Configuration=Debug /p:Platform=Win32
& 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe' .\Windows\NexPad.sln /p:Configuration=Debug /p:Platform=x64
```

If release-facing assets or docs are changed materially during implementation, repeat for release targets before closing the work.

## Runtime Setup

1. Launch the newly built `NexPad.exe` from `debug/x32/` or `debug/x64/`.
2. Confirm `config.ini` remains next to the executable.
3. In the Settings tab, leave the existing touchpad feature enabled.
4. Open a scrollable Windows target.

## Manual Validation Matrix

### USB DualSense

1. Connect the DualSense over USB.
2. Verify one finger still moves the cursor.
3. Verify a short one-finger tap still produces a left click.
4. Perform a two-finger vertical gesture and confirm vertical scroll.
5. Perform a two-finger horizontal gesture in a target that supports horizontal wheel input and confirm horizontal scroll.
6. Return to stick scrolling and confirm it behaves exactly as before.
7. Lift one finger before the other and confirm scrolling stops immediately with no extra click or wheel output.

### Bluetooth DualSense With Enhanced Reports

1. Connect the DualSense over Bluetooth.
2. Verify the same one-finger cursor and tap behavior.
3. Repeat two-finger vertical and horizontal scroll checks.
4. Pause mid-gesture, change direction, and resume to confirm stable behavior.

### Bluetooth Limited-Report Fallback

1. Exercise any environment where NexPad falls back to the limited Bluetooth report mode.
2. Confirm NexPad does not synthesize two-finger scroll from incomplete touch data.
3. Confirm unaffected controls still work, including stick scrolling and any supported one-finger touch behavior.

### Disconnect And Recovery Safety

1. Begin a two-finger gesture and disconnect the controller.
2. Confirm wheel output stops immediately and no click is generated.
3. Reconnect over the same transport and re-test.
4. Repeat while switching between USB and Bluetooth.
5. Repeat after a brief idle or sleep interval if available in the test setup.

### Non-DualSense Regression

1. Connect an XInput controller.
2. Confirm existing cursor movement and stick scrolling remain unchanged.
3. Confirm no touchpad-specific logic leaks into non-DualSense behavior.

## Config And Documentation Checks

1. If no new setting was introduced, confirm `TOUCHPAD_ENABLED`, `TOUCHPAD_DEAD_ZONE`, `TOUCHPAD_SPEED`, and `SCROLL_SPEED` still describe their original meaning.
2. If wording changed, confirm the same description appears in:
   - `Windows/NexPad/ConfigFile.cpp`
   - `Configs/config_default.ini`
   - shipped `config.ini` assets when applicable
   - `README.md`
   - release notes if behavior changes are release-facing
3. If implementation unexpectedly requires a new user-facing setting, block completion until config generation, load/save, preset import/export, UI, and docs all agree.

## Expected Outcome

- Two-finger scrolling works on reliable DualSense touch reports.
- One-finger cursor movement and tap-to-click remain intact.
- Stick scrolling remains unchanged.
- Disconnects, reconnects, and report downgrades produce no phantom cursor, click, or scroll output.