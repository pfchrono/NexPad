# Feature Specification: DualSense Two-Finger Scrolling

**Feature Branch**: `[001-dualsense-two-finger-scroll]`  
**Created**: 2026-03-21  
**Status**: Draft  
**Input**: User description: "Add DualSense two-finger scrolling to NexPad."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Scroll with two fingers on DualSense (Priority: P1)

As a DualSense user, I want to scroll Windows content with a two-finger touchpad gesture so I can navigate documents and web pages without giving up NexPad's existing controller mappings.

**Why this priority**: Two-finger scrolling is the requested user value and should work without forcing users to remap their sticks or abandon current input habits.

**Independent Test**: Connect a DualSense with touchpad input available, open a scrollable Windows view, perform a two-finger gesture, and confirm that the view scrolls while existing stick scroll still works before and after the gesture.

**Acceptance Scenarios**:

1. **Given** NexPad is enabled and a DualSense touchpad report exposes two active touches, **When** the user drags two fingers vertically on the touchpad, **Then** the active Windows view scrolls vertically.
2. **Given** NexPad is enabled and the current target accepts horizontal wheel input, **When** the user drags two fingers horizontally on the touchpad, **Then** the active Windows view scrolls horizontally.
3. **Given** the user has existing thumbstick scrolling configured, **When** the user returns to stick scrolling after a two-finger gesture, **Then** stick scrolling behaves the same as before.

---

### User Story 2 - Preserve current touchpad cursor and tap behavior (Priority: P2)

As an existing NexPad user, I want one-finger touchpad movement and tap-to-click to keep working so the new gesture adds capability without breaking learned behavior.

**Why this priority**: The repository already ships touchpad movement and tap-to-click. Regressing those paths would make the feature unsafe for existing users.

**Independent Test**: With touchpad support enabled, verify that one finger still moves the cursor, a short single-finger tap still clicks, and a two-finger scroll gesture does not produce accidental pointer movement or click output from that gesture.

**Acceptance Scenarios**:

1. **Given** a DualSense touchpad is available, **When** the user moves one finger on the touchpad, **Then** cursor movement continues and scrolling is not triggered.
2. **Given** a short single-finger tap with no drag, **When** the user releases the touchpad, **Then** NexPad produces the same left-click behavior it already provides today.
3. **Given** the user is performing a two-finger scroll gesture, **When** the gesture is active, **Then** NexPad does not treat that gesture as a tap-to-click action.

---

### User Story 3 - Recover safely across transport and report changes (Priority: P3)

As a DualSense user, I want scrolling to stop cleanly when touch data ends or changes so NexPad never leaves stale or phantom scroll input active during USB, Bluetooth, or reconnect transitions.

**Why this priority**: Controller input changes must be safe before they are clever. This repository explicitly requires protection against stale and phantom inputs.

**Independent Test**: Validate gesture start and stop on USB and Bluetooth DualSense connections, then repeat while lifting fingers unevenly, disconnecting, reconnecting, or falling back to limited touch reporting to confirm scrolling halts cleanly with no extra events.

**Acceptance Scenarios**:

1. **Given** a DualSense is connected over USB, **When** the user starts and then ends a two-finger scroll gesture, **Then** scrolling starts and stops promptly with no extra wheel output after the gesture ends.
2. **Given** a DualSense is connected over Bluetooth and touchpad reports still provide two-touch detail, **When** the user performs the same gesture, **Then** the behavior matches the USB path.
3. **Given** touchpad data becomes unavailable because the controller disconnects, reconnects, sleeps, or falls back to a limited report mode, **When** input resumes, **Then** NexPad emits no phantom scroll or click from the prior gesture and all unaffected controls continue to work.

### Edge Cases

- Bluetooth input falls back to a report mode that does not expose reliable two-touch detail.
- The user adds or removes the second finger partway through a gesture.
- The user lifts one finger before the other, pauses briefly, or changes direction mid-gesture.
- The controller disconnects, reconnects, or sleeps while a two-finger scroll is active.
- Touchpad support is disabled while stick scrolling remains enabled.
- The current Windows target accepts vertical wheel input but ignores horizontal wheel input.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST recognize an intentional two-finger DualSense touchpad gesture as a scroll interaction distinct from existing one-finger pointer movement.
- **FR-002**: System MUST convert two-finger vertical movement into vertical scroll output.
- **FR-003**: System MUST convert two-finger horizontal movement into horizontal scroll output when the active target accepts horizontal wheel input.
- **FR-004**: System MUST preserve existing thumbstick-based scrolling behavior, including any configured thumbstick swap behavior, before, during, and after touchpad scrolling.
- **FR-005**: System MUST preserve existing one-finger DualSense touchpad cursor movement whenever a second finger is not part of an active scroll gesture.
- **FR-006**: System MUST preserve the existing short single-finger tap-to-click behavior and MUST prevent two-finger scrolling from generating accidental tap clicks.
- **FR-007**: System MUST support the feature on both USB and Bluetooth DualSense connection paths when the available touch reports expose enough information to distinguish two simultaneous touches.
- **FR-008**: System MUST fail safe when touch reports are incomplete, stale, or lost by clearing active scroll state immediately and emitting no phantom cursor, click, or scroll input from prior touch data.
- **FR-009**: System MUST stop two-finger scrolling promptly when either touch is released, touchpad input is disabled, the controller disconnects, or the connection path changes.
- **FR-010**: System MUST leave non-DualSense controller behavior unchanged.
- **FR-011**: System MUST make two-finger scrolling available without requiring users to disable existing stick scrolling or existing one-finger touchpad movement.
- **FR-012**: If the feature introduces any new user-facing setting, the same setting and default value MUST be aligned across live settings, executable-relative config files, shared presets or defaults, and user documentation.

### Key Entities *(include if feature involves data)*

- **Touchpad Interaction Mode**: The current user intent for DualSense touch input, including no touch, one-finger pointer movement, and two-finger scrolling.
- **Transport Capability**: The active DualSense connection context that determines whether two-touch information is available, including USB, Bluetooth with full touch detail, and Bluetooth with limited touch detail.
- **Touchpad Preferences**: Existing touchpad enable and tuning choices, plus any optional future toggle for two-finger scrolling if a separate control is later deemed necessary.

## Assumptions

- Single-finger touchpad movement remains the default pointer gesture and two-finger scrolling is additive rather than a replacement.
- The existing touchpad enable setting governs touchpad-based pointer movement, tap-to-click, and two-finger scrolling unless later planning proves a separate toggle is necessary.
- When a Bluetooth report mode cannot reliably distinguish two simultaneous touches, the safe outcome is to preserve existing pointer and stick behavior without synthesizing two-finger scrolling.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: During manual validation with a supported DualSense, users successfully scroll a standard vertically scrollable Windows view with a two-finger gesture in at least 19 out of 20 attempts.
- **SC-002**: During manual regression validation over one USB session and one Bluetooth session, existing stick scrolling, one-finger touchpad cursor movement, and single-finger tap-to-click complete without observed regressions in 100% of scripted checks.
- **SC-003**: Across 20 gesture-end, disconnect, reconnect, sleep, or limited-report fallback transitions, users observe 0 unintended scroll or click events after the gesture has ended.
- **SC-004**: In a single validation session, users can switch between one-finger cursor movement and two-finger scrolling in under 1 second between gestures without restarting NexPad or remapping existing controls.
