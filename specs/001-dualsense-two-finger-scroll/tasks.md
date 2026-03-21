---

description: "Task breakdown for DualSense two-finger scrolling"
---

# Tasks: DualSense Two-Finger Scrolling

**Input**: Design documents from `/specs/001-dualsense-two-finger-scroll/`
**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/dualsense-touchpad-interaction.md, quickstart.md

**Tests**: No new automated test harness is specified for this feature. Validation for these tasks relies on the required MSBuild runs and the manual controller checks defined in `specs/001-dualsense-two-finger-scroll/quickstart.md`.

**Organization**: Tasks are grouped by user story so each increment can be implemented and validated against the feature spec.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependency on incomplete tasks)
- **[Story]**: Which user story this task belongs to (`[US1]`, `[US2]`, `[US3]`)
- Every task names the exact file path(s) it touches

## Path Conventions

- **Core application**: `Windows/NexPad/` for native C++ sources, headers, resources, and project files
- **Documentation and runtime assets**: `README.md`, `RELEASE_CHECKLIST.md`, `Configs/`, `debug/`, and `release/`
- **Feature artifacts**: `specs/001-dualsense-two-finger-scroll/`

## Phase 1: Setup (Shared Context)

**Purpose**: Confirm the exact brownfield surfaces, validation workflow, and configuration alignment before implementation starts.

- [ ] T001 Review `specs/001-dualsense-two-finger-scroll/plan.md`, `specs/001-dualsense-two-finger-scroll/spec.md`, `specs/001-dualsense-two-finger-scroll/data-model.md`, `specs/001-dualsense-two-finger-scroll/research.md`, and `specs/001-dualsense-two-finger-scroll/contracts/dualsense-touchpad-interaction.md` and lock the implementation scope for `Windows/NexPad/CXBOXController.h`, `Windows/NexPad/CXBOXController.cpp`, `Windows/NexPad/NexPad.h`, and `Windows/NexPad/NexPad.cpp`
- [ ] T002 [P] Confirm the required build commands and manual controller validation matrix in `specs/001-dualsense-two-finger-scroll/quickstart.md`
- [ ] T003 [P] Verify that the initial implementation keeps the existing touchpad setting surface aligned across `Windows/NexPad/ConfigFile.cpp`, `Configs/config_default.ini`, `config.ini`, and `README.md`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Establish the shared controller and interaction state required by every story.

**⚠️ CRITICAL**: No user story work should start until this phase is complete.

- [ ] T004 Expand the public touchpad model in `Windows/NexPad/CXBOXController.h` to represent two contacts, active finger count, and reliable two-touch capability
- [ ] T005 [P] Rework DualSense touch parsing and transport capability tracking in `Windows/NexPad/CXBOXController.cpp` for USB, Bluetooth enhanced, and Bluetooth simple report paths
- [ ] T006 [P] Add `Idle`, `OneFingerCursor`, and `TwoFingerScroll` interaction bookkeeping plus reset helpers in `Windows/NexPad/NexPad.h` and `Windows/NexPad/NexPad.cpp`
- [ ] T007 Wire shared touch-state reset behavior across disconnect, stale-report timeout, touch-disable, and transport downgrade in `Windows/NexPad/CXBOXController.cpp` and `Windows/NexPad/NexPad.cpp`

**Checkpoint**: Shared touch parsing and interaction state are ready for story work.

---

## Phase 3: User Story 1 - Scroll with two fingers on DualSense (Priority: P1) 🎯 MVP

**Goal**: Add reliable two-finger vertical and horizontal scroll output for DualSense touchpad input without disturbing existing stick scroll.

**Independent Test**: Connect a DualSense with reliable touch reports, perform a two-finger gesture in a scrollable Windows target, and confirm vertical or horizontal scrolling works while stick scrolling still works before and after the gesture.

### Implementation for User Story 1

- [ ] T008 [US1] Update `CXBOXController::GetTouchpadState` in `Windows/NexPad/CXBOXController.cpp` to expose reliable two-finger deltas and capability derived from the richer parser state
- [ ] T009 [US1] Implement gesture entry and arbitration for two reliable touches in `Windows/NexPad/NexPad.cpp` so touch input can switch from cursor mode to scroll mode
- [ ] T010 [US1] Convert two-finger vertical and horizontal touch deltas into `MOUSEEVENTF_WHEEL` and `MOUSEEVENTF_HWHEEL` output scaled by `SCROLL_SPEED` in `Windows/NexPad/NexPad.cpp`
- [ ] T011 [US1] Keep thumbstick scrolling independent during and after touchpad scroll interactions in `Windows/NexPad/NexPad.cpp`

**Checkpoint**: Two-finger touchpad scroll works as the MVP behavior.

---

## Phase 4: User Story 2 - Preserve current touchpad cursor and tap behavior (Priority: P2)

**Goal**: Keep one-finger cursor motion and tap-to-click intact while the new two-finger gesture is active and additive.

**Independent Test**: Verify one-finger movement still moves the cursor, a short one-finger tap still left-clicks, and a two-finger scroll gesture does not move the cursor or trigger tap-to-click.

### Implementation for User Story 2

- [ ] T012 [US2] Keep one-finger cursor movement on the existing `TOUCHPAD_SPEED` and `TOUCHPAD_DEAD_ZONE` path in `Windows/NexPad/NexPad.cpp` whenever the interaction mode is not `TwoFingerScroll`
- [ ] T013 [US2] Update tap eligibility and release handling in `Windows/NexPad/NexPad.cpp` so a short single-finger tap still clicks and any two-finger gesture cancels tap-to-click
- [ ] T014 [P] [US2] Refresh generated and default touchpad setting comments in `Windows/NexPad/ConfigFile.cpp`, `Configs/config_default.ini`, and `config.ini` to describe additive cursor plus two-finger scroll behavior
- [ ] T015 [P] [US2] Update user-facing wording in `README.md` and `Windows/NexPad/main.cpp` so the Settings tab and docs match the unchanged touchpad setting surface

**Checkpoint**: One-finger cursor and tap behavior remain intact alongside two-finger scroll.

---

## Phase 5: User Story 3 - Recover safely across transport and report changes (Priority: P3)

**Goal**: Ensure touch-driven scrolling stops cleanly and never produces stale or phantom input across disconnects, reconnects, finger-count changes, or degraded report modes.

**Independent Test**: Start and stop gestures across USB and Bluetooth, then repeat while releasing fingers unevenly, disconnecting, reconnecting, or falling back to limited reports and confirm there is no phantom scroll or click.

### Implementation for User Story 3

- [ ] T016 [US3] Require reliable two-touch capability before entering `TwoFingerScroll` in `Windows/NexPad/CXBOXController.cpp` and `Windows/NexPad/NexPad.cpp`
- [ ] T017 [US3] Clear active scroll, cursor, and tap interaction state immediately on finger-count changes or touch loss in `Windows/NexPad/NexPad.cpp`
- [ ] T018 [US3] Preserve Bluetooth simple-report fallback as a no-scroll, no-phantom-input path in `Windows/NexPad/CXBOXController.cpp`
- [ ] T019 [US3] Keep non-DualSense and DualShock4 behavior unchanged when reliable two-touch data is unavailable in `Windows/NexPad/CXBOXController.cpp` and `Windows/NexPad/NexPad.cpp`

**Checkpoint**: Gesture safety holds across transport changes, stale data, and unsupported controllers.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Validate the complete feature against build, runtime asset, and release workflow expectations.

- [ ] T020 Run `Debug|Win32` and `Debug|x64` MSBuild validation for `Windows/NexPad.sln` using the commands in `specs/001-dualsense-two-finger-scroll/quickstart.md`
- [ ] T021 Execute the USB DualSense, Bluetooth enhanced, Bluetooth limited-report fallback, disconnect or reconnect, and non-DualSense checks in `specs/001-dualsense-two-finger-scroll/quickstart.md`
- [ ] T022 Sync executable-relative config copies in `debug/x32/config.ini`, `debug/x64/config.ini`, `release/x32/config.ini`, and `release/x64/config.ini` if touchpad setting text or defaults changed
- [ ] T023 Run `scripts/package-release.ps1` and `scripts/verify-release-checksums.ps1` and confirm `RELEASE_CHECKLIST.md` still matches the output layout if runtime assets or release-facing documentation changed

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1: Setup**: No dependencies; start immediately.
- **Phase 2: Foundational**: Depends on Phase 1; blocks all story work.
- **Phase 3: User Story 1**: Depends on Phase 2.
- **Phase 4: User Story 2**: Depends on Phase 2 and should validate against the interaction mode introduced for User Story 1.
- **Phase 5: User Story 3**: Depends on Phase 2 and reaches final value once the User Story 1 scroll path exists.
- **Phase 6: Polish**: Depends on all desired user stories being complete.

### User Story Dependencies

- **US1**: First deliverable after the foundational parser and interaction state work.
- **US2**: Can begin after the foundational phase, but final verification must confirm it remains compatible with the US1 scroll path.
- **US3**: Can begin after the foundational phase, but final validation depends on the US1 two-finger scroll behavior being present.

### Within Each User Story

- Expose reliable state before changing gesture behavior.
- Change interaction arbitration before touching docs or wording.
- Finish runtime behavior before build and hardware validation.
- Keep config, docs, and runtime assets aligned whenever user-facing wording changes.

## Parallel Opportunities

- `T002` and `T003` can run in parallel after `T001`.
- `T005` and `T006` can run in parallel after `T004`.
- `T014` and `T015` can run in parallel after the US2 runtime behavior is stable.

## Parallel Example: User Story 1

User Story 1 is mostly sequential once implementation begins because `T009`, `T010`, and `T011` all modify `Windows/NexPad/NexPad.cpp`. The only safe parallel split is finishing `T008` in `Windows/NexPad/CXBOXController.cpp` while another developer prepares the implementation approach for `Windows/NexPad/NexPad.cpp` without merging incomplete changes.

## Parallel Example: User Story 2

After `T012` and `T013` stabilize the runtime behavior, these tasks can run in parallel:

- `T014` in `Windows/NexPad/ConfigFile.cpp`, `Configs/config_default.ini`, and `config.ini`
- `T015` in `README.md` and `Windows/NexPad/main.cpp`

## Parallel Example: User Story 3

User Story 3 is also mostly sequential because its safety work spans shared controller and interaction reset paths. The practical parallelism is to implement `T018` in `Windows/NexPad/CXBOXController.cpp` while another developer reviews the `T017` reset transitions in `Windows/NexPad/NexPad.cpp`, then merge only after the reset contract is agreed.

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup.
2. Complete Phase 2: Foundational.
3. Complete Phase 3: User Story 1.
4. Validate the two-finger scroll path with the quickstart build and manual checks.
5. Stop and confirm the MVP before broadening the change surface.

### Incremental Delivery

1. Build the shared parser and interaction state.
2. Deliver US1 for two-finger scroll.
3. Add US2 regression protection for cursor and tap behavior.
4. Add US3 transport and recovery safety.
5. Finish with build validation, hardware validation, and asset alignment.

### Parallel Team Strategy

1. One developer completes `T004`.
2. Split `T005` and `T006` across controller parsing and NexPad interaction work.
3. Deliver US1 first to anchor the gesture contract.
4. Once US1 is stable, split US2 wording and config alignment from US3 safety work.

---

## Notes

- The plan assumes no new user-facing setting is introduced for two-finger scrolling.
- If implementation proves a new setting is necessary, add follow-up tasks for `Windows/NexPad/ConfigFile.cpp`, `Configs/config_default.ini`, `config.ini`, `Windows/NexPad/main.cpp`, `README.md`, and any shipped runtime config copies before implementation is considered complete.
- Bluetooth limited-report mode remains a fail-safe no-scroll path unless reliable two-touch data is available.
- Controller-affecting changes are not done until both build validation and manual hardware checks pass.