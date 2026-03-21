# Research: DualSense Two-Finger Scrolling

This document resolves the design unknowns identified while planning the feature against the current NexPad codebase.

## Decision 1: Extend the controller touch state to expose reliable two-contact data

**Decision**: Broaden `CXBOXController::TouchpadState` and the DualSense parsing path so the controller layer can report enough information to distinguish one-finger cursor movement from intentional two-finger scroll.

**Rationale**: The current implementation only exposes `active`, `available`, `deltaX`, and `deltaY` for a single contact. That is sufficient for one-finger cursor movement but not for deciding whether a second simultaneous touch exists or whether the current transport can be trusted to provide that detail. The parser already owns transport detection, Bluetooth enhanced-request logic, stale-report handling, and touch reset behavior, so it is the correct place to expose richer touch state.

**Alternatives considered**:

- Infer two-finger intent inside `NexPad.cpp` from current one-finger deltas. Rejected because the current state does not expose second-touch presence or reliability.
- Emit wheel events directly from `CXBOXController.cpp`. Rejected because input arbitration and `SendInput` output already live in `NexPad.cpp`, and moving that behavior down would spread output logic across layers.

## Decision 2: Keep gesture arbitration in `NexPad.cpp`

**Decision**: Add a small touch interaction mode in `NexPad` that decides between idle, one-finger cursor, and two-finger scroll behavior while preserving the existing loop order and output ownership.

**Rationale**: `NexPad::loop()` already sequences cursor movement, stick scrolling, tap-to-click, and button mappings. That is the layer that can safely suppress cursor movement and tap output during two-finger scroll without affecting stick-based scroll or unrelated controller mappings. The controller layer should remain focused on extracting safe state from reports.

**Alternatives considered**:

- Reorder the loop and let tap handling infer whether scroll happened earlier. Rejected because it is brittle and does not solve the missing interaction state.
- Disable all touchpad behavior whenever two fingers are seen. Rejected because the feature requires scroll output, not a no-op.

## Decision 3: Bluetooth support stays capability-driven

**Decision**: Support two-finger scrolling only when the active report path exposes reliable two-touch detail, and fail safe when Bluetooth falls back to limited reports.

**Rationale**: The current code already requests enhanced DualSense reports and already resets touchpad state when Bluetooth falls back to the simple report path. That existing behavior matches the feature spec's safety requirements: when two-touch detail is not trustworthy, NexPad must avoid synthesizing cursor, click, or scroll output from stale or incomplete touch data.

**Alternatives considered**:

- Guess two-finger gestures from limited Bluetooth reports. Rejected because there is no reliable signal for two simultaneous touches in the simple report path.
- Disable all touchpad behavior on Bluetooth. Rejected because enhanced Bluetooth reports can carry valid touchpad detail and the feature explicitly calls for support when sufficient data exists.

## Decision 4: Do not add a new user-facing setting in the initial design

**Decision**: Keep `TOUCHPAD_ENABLED` as the existing top-level gate for touchpad-derived behavior and do not introduce a dedicated two-finger scroll toggle for the first implementation pass.

**Rationale**: The repository already threads touchpad settings through config generation, config load/save, preset import/export, the Settings tab, and README documentation. Adding another user-facing toggle would expand the change surface significantly without a requirement that users need an independent opt-out. The feature spec already assumes that touchpad behavior remains additive under the current touchpad enable setting unless planning proves otherwise, and current code evidence does not justify that expansion.

**Alternatives considered**:

- Add `TOUCHPAD_SCROLL_ENABLED`. Rejected because it increases brownfield surface area across config, UI, presets, and docs without solving a demonstrated problem.
- Add separate sensitivity controls at the same time. Rejected because the current request is about behavior, not a new tuning system.

## Decision 5: Preserve the meaning of current speed settings

**Decision**: Keep `TOUCHPAD_SPEED` dedicated to one-finger cursor movement and derive touchpad scroll output from internal normalization combined with the existing `SCROLL_SPEED` behavior.

**Rationale**: Existing docs and settings text define `TOUCHPAD_SPEED` as cursor sensitivity. Reusing it for two-finger wheel output would silently change a published setting's meaning. Using `SCROLL_SPEED` as the user-visible scroll scalar keeps scroll semantics aligned with the rest of NexPad while leaving room for an internal conversion factor tailored to touch deltas.

**Alternatives considered**:

- Reuse `TOUCHPAD_SPEED` for both cursor and scroll. Rejected because it would blur two separate behaviors and make current documentation inaccurate.
- Introduce `TOUCHPAD_SCROLL_SPEED`. Rejected for the same minimal-surface reasons as Decision 4.

## Result

All planning clarifications are resolved:

- The controller layer will expose reliable two-touch capability and contact state.
- `NexPad` will own gesture arbitration and safety resets.
- Bluetooth simple fallback remains no-scroll and fail-safe.
- No new user-facing setting is required in the initial design.
- Existing config semantics remain stable.