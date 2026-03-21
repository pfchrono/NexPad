Controller Compatibility and Troubleshooting
=================

This guide describes NexPad controller compatibility by connection path, current validation confidence, known limitations, and troubleshooting steps.

Compatibility Matrix
======

Status legend:

* **Release-validated**: Explicitly validated in release notes or scripted manual checks.
* **Implemented**: Code path exists in the current repository, but no model-specific release validation is currently documented.
* **Limited**: Supported with reduced capability in some transport scenarios.

| Controller / Input Path | Connection | Current Status | Notes |
|---|---|---|---|
| XInput-compatible controllers (Xbox 360 / Xbox One / Xbox Series / compatible third-party XInput) | USB or wireless through XInput | Implemented | NexPad loads `xinput1_4.dll`, `xinput1_3.dll`, or `xinput9_1_0.dll` and maps standard XInput state. |
| DualSense (PID `0x0CE6`) via native HID fallback | USB HID | Release-validated | Touchpad cursor support and touch gesture work were hardware-validated in the v0.1.3 and v0.1.4 release cycle. |
| DualSense Edge (PID `0x0DF2`) via native HID fallback | USB HID | Implemented | Uses the same DualSense HID path; validate on target hardware before production use. |
| DualSense / DualSense Edge via native HID fallback | Bluetooth HID | Limited | NexPad requests enhanced reports first, then falls back to simplified Bluetooth parsing if full touch data is unavailable. Two-finger gesture reliability depends on available report detail. |
| DualShock 4 v1 (PID `0x05C4`) via native HID fallback | USB/Bluetooth HID | Implemented | Parsed through the PlayStation HID path and mapped into XInput-style state. |
| DualShock 4 v2 (PID `0x09CC`) via native HID fallback | USB/Bluetooth HID | Implemented | Same as DualShock 4 v1 path. |

Known Limitations
======

* The native HID fallback currently targets Sony vendor ID `0x054C` with known product IDs listed above.
* On some Bluetooth DualSense sessions, full touch detail may not be available; NexPad can still run with simplified input parsing.
* Third-party controllers are only expected to work when they expose a reliable XInput surface.

Troubleshooting
======

Controller not detected
------

1. Open NexPad and verify the Status tab shows controller connection changes.
2. Reconnect the controller and wait a few seconds for polling/discovery.
3. For non-Sony controllers, confirm they present as XInput in Windows.
4. If another remapping layer is active, close it and retest to avoid input-path conflicts.

DualSense on Bluetooth has reduced touch behavior
------

1. This can happen when enhanced Bluetooth reports are unavailable.
2. Keep one-finger cursor and core mappings enabled; treat two-finger touch behavior as transport-dependent in this mode.
3. If full touch behavior is required, use USB HID for the session.

PlayStation controller appears connected but behavior is inconsistent
------

1. Confirm the controller model is one of the currently mapped Sony product IDs.
2. Restart NexPad after reconnecting the controller.
3. If using Bluetooth, retest over USB to distinguish transport limitations from mapping problems.

Third-party controller maps incorrectly
------

1. Validate whether the device is exposing XInput rather than DirectInput-only mode.
2. Test with default NexPad config before applying custom mappings.
3. If button layout is non-standard, use the Mappings tab to adjust values and save a preset profile.

Release Hardware Validation Matrix
======

Use this matrix when preparing releases. Mark each row as `PASS`, `FAIL`, or `N/A`.

| Scenario | Required For Release | Result | Notes |
|---|---|---|---|
| XInput controller connects and controls cursor/click/scroll | Yes |  |  |
| XInput disable toggle and re-enable path works | Yes |  |  |
| DualSense USB: one-finger cursor and tap behavior | Yes (if touchpad features are in scope) |  |  |
| DualSense USB: two-finger scroll behavior | Yes (if touchpad features are in scope) |  |  |
| DualSense Bluetooth: app remains stable and usable with current report mode | Yes (if DualSense is in scope) |  |  |
| DualShock 4 HID basic mapping behavior | Optional (recommended when hardware is available) |  |  |

Keep this matrix synchronized with `RELEASE_CHECKLIST.md` and release notes for each tagged release.
