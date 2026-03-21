# Contract: DualSense Touchpad Interaction

## Scope

This contract defines the user-visible behavior and configuration expectations for DualSense touchpad input after two-finger scrolling is added.

## Inputs

- Controller type reported by the existing controller layer.
- Current transport capability: USB, Bluetooth enhanced, Bluetooth limited, or unavailable.
- Reliable touch contact data from the DualSense HID parser.
- Existing settings: `TOUCHPAD_ENABLED`, `TOUCHPAD_DEAD_ZONE`, `TOUCHPAD_SPEED`, `SCROLL_SPEED`, and `SWAP_THUMBSTICKS`.

## Output Rules

### Rule 1: One finger remains cursor-only

- When exactly one reliable touch is active and touchpad input is enabled, NexPad moves the cursor using the existing touchpad cursor path.
- `TOUCHPAD_DEAD_ZONE` and `TOUCHPAD_SPEED` continue to apply to this cursor path.
- A short release without meaningful pointer motion remains eligible for the existing tap-to-click behavior.

### Rule 2: Two reliable fingers become scroll-only

- When two reliable simultaneous touches are active and the current report path supports two-touch detail, NexPad treats the interaction as scrolling.
- Vertical movement produces `MOUSEEVENTF_WHEEL` output.
- Horizontal movement produces `MOUSEEVENTF_HWHEEL` output when the target supports horizontal wheel input.
- While this mode is active, NexPad must suppress touchpad-driven cursor movement and suppress tap-to-click.

### Rule 3: Stick scrolling remains independent

- Thumbstick-based scrolling continues to use the existing stick path and `SCROLL_SPEED` behavior.
- `SWAP_THUMBSTICKS` continues to affect only stick-based mouse and scroll assignment.
- Two-finger touch scrolling must not disable or remap existing stick scrolling.

### Rule 4: Safety overrides all touch-derived output

- If either finger is released during a two-finger scroll interaction, touchpad scrolling stops immediately.
- If touchpad input becomes unavailable, the controller disconnects, transport changes, or reports become stale or incomplete, NexPad clears active touch-derived scroll and tap state immediately.
- NexPad must not emit phantom cursor, click, or scroll output from touch data that is no longer valid.

### Rule 5: Limited Bluetooth reports do not synthesize two-finger scroll

- If Bluetooth falls back to a report mode that does not expose reliable two-touch data, NexPad must not guess or infer two-finger scroll.
- In that state, behavior must fail safe and preserve unaffected controls.

### Rule 6: Non-DualSense controllers remain unchanged

- XInput controllers and non-DualSense devices continue using existing behavior with no touchpad-derived scroll path.

## Configuration Contract

- The initial implementation does not add a new user-facing setting.
- `TOUCHPAD_ENABLED` remains the gate for touchpad-derived behavior.
- `TOUCHPAD_SPEED` remains a cursor-movement setting.
- `SCROLL_SPEED` remains the user-visible scroll scalar.
- If a future implementation introduces a new touchpad scroll-specific setting, that change must be applied consistently to config generation, config load/save, presets, Settings UI, and documentation before release.

## Documentation Contract

If user-facing wording changes during implementation, the same behavior description must be reflected in:

- `README.md`
- generated config text from `Windows/NexPad/ConfigFile.cpp`
- default config assets under `Configs/` and shipped runtime config assets when updated
- release notes if the behavior is part of a shipped release