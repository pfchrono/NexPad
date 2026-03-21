# Data Model: DualSense Two-Finger Scrolling

## Overview

The feature extends the current one-finger DualSense touchpad pipeline into a small, explicit model for touch contacts, gesture intent, transport capability, and user preferences.

## Entity: Touch Contact Snapshot

Represents one touch contact extracted from the current DualSense report.

**Fields**:

- `isActive`: Whether the contact is currently present in the report.
- `x`: Current touch X coordinate in controller touchpad space.
- `y`: Current touch Y coordinate in controller touchpad space.
- `previousX`: Previous X coordinate used to derive delta.
- `previousY`: Previous Y coordinate used to derive delta.
- `deltaX`: Per-report horizontal movement.
- `deltaY`: Per-report vertical movement.

**Validation rules**:

- Delta values are valid only when the same contact was active in the prior report.
- When a contact becomes inactive, its position and delta values must reset immediately.
- Contact data must not survive disconnect, stale-report timeout, or transport downgrade.

## Entity: Touchpad Interaction Mode

Represents the current user intent for DualSense touch input inside `NexPad`.

**States**:

- `Idle`: No active reliable touch interaction.
- `OneFingerCursor`: A single reliable touch is driving pointer movement and may still qualify for tap-to-click on release.
- `TwoFingerScroll`: Two reliable simultaneous touches are driving wheel output and suppress cursor movement plus tap-to-click.

**Fields**:

- `mode`: One of the three states above.
- `activeFingerCount`: Number of reliable active touches in the current frame.
- `tapEligible`: Whether the current interaction may still become a tap on release.
- `tapStartTick`: Start time of the current one-finger tap candidate.
- `scrollResidualX`: Optional accumulated remainder for horizontal wheel normalization.
- `scrollResidualY`: Optional accumulated remainder for vertical wheel normalization.

**State transitions**:

- `Idle -> OneFingerCursor` when one reliable touch begins.
- `Idle -> TwoFingerScroll` when two reliable touches begin and transport supports two-touch detail.
- `OneFingerCursor -> TwoFingerScroll` when a second reliable touch joins before release.
- `OneFingerCursor -> Idle` when the only active touch releases.
- `TwoFingerScroll -> Idle` when either touch releases, touch input becomes unavailable, transport downgrades, or the controller disconnects.

## Entity: Transport Capability

Represents whether the active controller connection can safely participate in two-finger scrolling.

**Fields**:

- `transportType`: `Usb`, `Bluetooth`, or `Unknown`.
- `reportMode`: `Usb`, `BluetoothEnhanced`, `BluetoothSimple`, or equivalent parser result.
- `supportsReliableTouch`: Whether touch data is currently available at all.
- `supportsReliableTwoTouch`: Whether the current report path exposes enough detail to distinguish two simultaneous touches safely.
- `lastValidReportTick`: Timestamp of the last valid parsed HID report.
- `staleTimeoutMs`: Time window after which cached touch-derived state must be cleared.

**Validation rules**:

- `supportsReliableTwoTouch` must be false for limited Bluetooth report paths.
- Any disconnect, read failure, or stale timeout clears touch-derived interaction state.
- Non-DualSense controllers never advertise reliable two-touch capability.

## Entity: Touchpad Preferences

Represents the user-facing settings that govern touch-derived behavior.

**Fields**:

- `touchpadEnabled`: Mirrors `TOUCHPAD_ENABLED` and gates all touchpad-derived behavior.
- `touchpadDeadZone`: Mirrors `TOUCHPAD_DEAD_ZONE` for one-finger pointer filtering.
- `touchpadSpeed`: Mirrors `TOUCHPAD_SPEED` for one-finger pointer sensitivity.
- `scrollSpeed`: Mirrors `SCROLL_SPEED` and remains the user-visible scroll scalar.
- `swapThumbsticks`: Mirrors `SWAP_THUMBSTICKS` and must continue to affect only stick-based mouse and scroll behavior.

**Validation rules**:

- Preference values load from executable-relative config data and must remain aligned across live settings, config persistence, and presets.
- `touchpadEnabled = 0` forces `Touchpad Interaction Mode` to `Idle` and disables both one-finger cursor and two-finger scroll.
- Touch-derived behavior must not change any setting semantics for non-touch features.

## Relationships

- `Transport Capability` determines whether `Touch Contact Snapshot` data is safe to use for one or two-finger interactions.
- `Touch Contact Snapshot` feeds `Touchpad Interaction Mode`.
- `Touchpad Preferences` gate and scale the behavior produced by `Touchpad Interaction Mode`.
- Stick scroll remains separate from touchpad interactions and continues to use the current thumbstick path regardless of touch mode.