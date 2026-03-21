---

description: "Task breakdown for the status version and battery UI feature"
---

# Tasks: Status Version And Battery UI

**Input**: Design documents from `/specs/002-status-version-battery-ui/`
**Prerequisites**: plan.md, spec.md

**Tests**: No new automated test harness is required by the spec. Validation relies on MSBuild runs, a packaging smoke test, and manual UI checks across controller and battery states.

**Organization**: Tasks are grouped by user story so each slice can be implemented and validated independently.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependency on incomplete tasks)
- **[Story]**: Which user story this task belongs to (`[US1]`, `[US2]`, `[US3]`)
- Every task names the exact file path(s) it touches

## Path Conventions

- **Core application**: `Windows/NexPad/` for native C++ sources, headers, resources, and project files
- **Build and packaging**: `scripts/` plus output validation through `debug/`, `release/`, and `artifacts/` when relevant
- **Documentation and runtime assets**: `README.md`, `RELEASE_CHECKLIST.md`, `Configs/`, `presets/`, and `docs/`
- **Feature artifacts**: `specs/002-status-version-battery-ui/`

## Phase 1: Setup (Shared Context)

**Purpose**: Confirm the exact status UI, version, and packaging surfaces before implementation starts.

- [ ] T001 Review `specs/002-status-version-battery-ui/spec.md` and `specs/002-status-version-battery-ui/plan.md` and lock scope for `Windows/NexPad/main.cpp`, `Windows/NexPad/CXBOXController.h`, `Windows/NexPad/CXBOXController.cpp`, `Windows/NexPad/NexPad.vcxproj`, `Windows/NexPad/resource.h`, `Windows/NexPad/Resource.rc`, and `scripts/package-release.ps1`
- [ ] T002 [P] Confirm the current tag and release version scheme used by the repository and `scripts/package-release.ps1`
- [ ] T003 [P] Review the current status page layout, tray tooltip updates, and battery text surfaces in `Windows/NexPad/main.cpp` and `Windows/NexPad/CXBOXController.cpp`

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Add the shared version and battery data surfaces needed by all user stories.

**⚠️ CRITICAL**: No user story work should start until this phase is complete.

- [ ] T004 Add a structured battery presentation model for the UI in `Windows/NexPad/CXBOXController.h`
- [ ] T005 [P] Populate the structured battery presentation model across XInput and HID controller paths in `Windows/NexPad/CXBOXController.cpp`
- [ ] T006 [P] Add a single app-version source for the native build and packaging flow in `Windows/NexPad/NexPad.vcxproj`, `scripts/package-release.ps1`, and a generated or checked-in version include under `Windows/NexPad/`
- [ ] T007 Add any shared status-page state or helper functions needed for the battery bar and footer version label in `Windows/NexPad/main.cpp`

**Checkpoint**: Version and battery data are available for UI integration.

---

## Phase 3: User Story 1 - Read Battery At A Glance (Priority: P1) 🎯 MVP

**Goal**: Replace the text-only battery row with a color-coded battery bar that still shows charging and clear fallback states.

**Independent Test**: Open the status page with supported connected and disconnected controller states and verify the battery row shows a readable bar, charging indicator, or fallback state.

### Implementation for User Story 1

- [ ] T008 [US1] Normalize battery charge buckets, charging state, and fallback labels for the UI in `Windows/NexPad/CXBOXController.cpp`
- [ ] T009 [US1] Implement the custom battery bar drawing and charging indicator rendering in `Windows/NexPad/main.cpp`
- [ ] T010 [US1] Replace the current battery text-only status row and update status-page layout in `Windows/NexPad/main.cpp`
- [ ] T011 [US1] Preserve coherent tray tooltip and non-graphical battery fallback text in `Windows/NexPad/main.cpp` and `Windows/NexPad/CXBOXController.cpp`

**Checkpoint**: The status page shows a working battery bar with charging and fallback behavior.

---

## Phase 4: User Story 2 - Confirm Running Build Version (Priority: P2)

**Goal**: Display a bottom-right `v-<version>` label that matches the active build or packaged release version.

**Independent Test**: Launch NexPad from a build or packaged artifact and confirm the status page shows the expected bottom-right version label.

### Implementation for User Story 2

- [ ] T012 [US2] Derive the displayable app version string in `v-<version>` format from the shared version source in the new version include and `scripts/package-release.ps1`
- [ ] T013 [US2] Add a bottom-right status-page version label or footer in `Windows/NexPad/main.cpp`
- [ ] T014 [US2] Keep the in-app version display aligned with packaged artifact version naming in `scripts/package-release.ps1` and `Windows/NexPad/NexPad.vcxproj`
- [ ] T015 [US2] Add executable version metadata in `Windows/NexPad/Resource.rc` and `Windows/NexPad/resource.h` if file properties are kept aligned with the same version source

**Checkpoint**: The app shows a stable bottom-right version label that matches the build version.

---

## Phase 5: User Story 3 - Preserve Existing Status Surfaces And Build Flow (Priority: P3)

**Goal**: Keep tray text, disconnected states, and packaging flow coherent after the new status UI is added.

**Independent Test**: Build and package NexPad, then verify the status UI, tray text, and disconnected battery states remain coherent.

### Implementation for User Story 3

- [ ] T016 [US3] Preserve disconnected, wired, unknown, and unavailable battery states without misleading colors or stale bars in `Windows/NexPad/main.cpp` and `Windows/NexPad/CXBOXController.cpp`
- [ ] T017 [US3] Ensure local builds without a release tag still produce a non-empty version label in `Windows/NexPad/NexPad.vcxproj`, the version include, and `Windows/NexPad/main.cpp`
- [ ] T018 [US3] Preserve existing tray tooltip readability and status refresh behavior in `Windows/NexPad/main.cpp`

**Checkpoint**: Status UI enhancements do not break disconnected states, local builds, or tray behavior.

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Validate the complete feature against build, packaging, and manual UI expectations.

- [ ] T019 Run `Debug|Win32` and `Debug|x64` MSBuild validation for `Windows/NexPad.sln`
- [ ] T020 Run a packaging smoke test through `scripts/package-release.ps1` with an explicit version value and verify artifact version naming stays aligned with the in-app version source
- [ ] T021 Manually validate battery bar colors, charging indicator, fallback states, and the bottom-right version label on the status page
- [ ] T022 Update `README.md` or release-facing notes if the new status UI and version label are considered user-visible release changes

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1: Setup**: No dependencies; start immediately.
- **Phase 2: Foundational**: Depends on Phase 1; blocks all story work.
- **Phase 3: User Story 1**: Depends on Phase 2.
- **Phase 4: User Story 2**: Depends on Phase 2.
- **Phase 5: User Story 3**: Depends on Phase 2 and validates the combined behavior from User Stories 1 and 2.
- **Phase 6: Polish**: Depends on all desired user stories being complete.

### User Story Dependencies

- **US1**: First deliverable after shared battery data and status-page helpers are ready.
- **US2**: Can begin after shared version plumbing exists.
- **US3**: Depends on the battery and version surfaces being in place so integration and fallback behavior can be validated.

### Within Each User Story

- Add shared data surfaces before UI rendering.
- Update UI refresh and layout together rather than separately.
- Keep build or package version alignment in place before validating the footer version label.
- Finish runtime behavior before final build and packaging validation.

## Parallel Opportunities

- `T002` and `T003` can run in parallel after `T001`.
- `T005` and `T006` can run in parallel after `T004`.
- `T009` and `T012` can proceed in parallel once foundational data surfaces exist, provided they do not merge conflicting edits to the same `main.cpp` regions without coordination.

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup.
2. Complete Phase 2: Foundational.
3. Complete Phase 3: User Story 1.
4. Validate the battery bar UI independently before expanding scope.

### Incremental Delivery

1. Add shared battery and version data surfaces.
2. Deliver the battery bar UI.
3. Add the bottom-right version footer.
4. Validate fallback states, tray behavior, and packaging alignment.

### Parallel Team Strategy

1. One developer implements structured battery presentation in `CXBOXController.*`.
2. Another developer wires the shared version source in `NexPad.vcxproj`, the version include, and `scripts/package-release.ps1`.
3. Merge both before the `main.cpp` status-page rendering work.

---

## Notes

- The new feature intentionally does not change controller input mappings.
- Battery state should be modeled structurally in code instead of parsed from user-facing strings.
- Version display should be sourced at build or package time, not by reading Git state at runtime.
- If executable file properties are not included in scope, `T015` can be dropped while keeping in-app and artifact version alignment intact.