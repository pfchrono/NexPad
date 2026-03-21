# Feature Specification: Status Version And Battery UI

**Feature Branch**: `[002-status-version-battery-ui]`  
**Created**: 2026-03-21  
**Status**: Draft  
**Input**: User description: "Add a version system that matches the current tag number and display it in the app bottom-right as v-<tag number>. Replace the battery text with a bar display that keeps charging visible and color-codes charge level."

> For NexPad brownfield features, specs MUST describe how the new behavior adds
> value without regressing existing controller input, configuration, build, or
> release behavior.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Read Battery At A Glance (Priority: P1)

As a NexPad user, I want the status page to show a battery bar with clear charge colors and charging state so I can understand controller power quickly without parsing text.

**Why this priority**: The battery display is the main user-visible status improvement and provides the clearest direct value from this feature.

**Independent Test**: Open the status page with a supported controller connected and verify the battery row shows a bar-based charge display with a readable charging indicator or fallback state.

**Acceptance Scenarios**:

1. **Given** a connected controller reports a known battery level, **When** the status page refreshes, **Then** NexPad shows a battery bar whose fill and color match the reported charge bucket.
2. **Given** a connected controller is charging, **When** the status page refreshes, **Then** NexPad shows the same battery bar plus a visible charging indicator.
3. **Given** battery information is unavailable, wired-only, or disconnected, **When** the status page refreshes, **Then** NexPad shows a clear non-misleading fallback state instead of stale battery bars.

---

### User Story 2 - Confirm Running Build Version (Priority: P2)

As a NexPad user or maintainer, I want the app to display its current version in the bottom-right corner so I can confirm the running build matches the intended release tag.

**Why this priority**: The version label helps confirm release alignment, but it is less critical than the at-a-glance battery status improvement.

**Independent Test**: Launch NexPad from a build or packaged artifact and verify the bottom-right status footer shows the expected `v-<version>` label.

**Acceptance Scenarios**:

1. **Given** NexPad is built or packaged with a known version value, **When** the main window opens, **Then** the status page shows that version in the bottom-right as `v-<version>`.
2. **Given** the build is produced from a local or non-release context, **When** the main window opens, **Then** NexPad still shows a non-empty fallback version label instead of blank or invalid text.
3. **Given** a packaged release artifact is created for a tagged version, **When** the user compares the UI version and artifact version, **Then** they match the same release value.

---

### User Story 3 - Preserve Existing Status Surfaces And Build Flow (Priority: P3)

As a maintainer, I want the new status UI to preserve existing controller status behavior and build packaging flow so the feature adds visibility without breaking tray text, disconnected states, or release packaging.

**Why this priority**: Version and battery visuals are useful only if they do not destabilize the existing brownfield app behavior and release flow.

**Independent Test**: Build NexPad for Win32 and x64, open the status page across connected and disconnected controller states, and confirm tray and packaging surfaces still behave coherently.

**Acceptance Scenarios**:

1. **Given** no controller is connected, **When** the status page refreshes, **Then** NexPad does not show stale battery bars or stale charging text.
2. **Given** the tray tooltip updates while the app is running, **When** controller state changes, **Then** the existing tooltip remains readable and does not depend on the new bar-only presentation.
3. **Given** the packaging flow is run with an explicit version value, **When** release artifacts are produced, **Then** the build completes successfully and the app version source remains aligned with the packaged version.

### Edge Cases

- A controller is connected through a path that reports only a coarse battery bucket rather than an exact percentage.
- A wired controller reports power state without a meaningful battery charge value.
- Battery status is unknown, unavailable, or temporarily stale while the controller remains connected.
- The controller disconnects while the status page is open.
- A build is produced from a local workspace without a new release tag.
- The status page is resized or displayed on a small window where the footer label and battery bar must still remain legible.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST display controller battery state on the status page as a bar-based visual indicator rather than battery text only.
- **FR-002**: System MUST color-code the displayed battery bar according to charge level.
- **FR-003**: System MUST display charging state alongside the battery bar when the controller reports charging.
- **FR-004**: System MUST show a clear fallback battery presentation when charge information is unavailable, unknown, wired-only, or disconnected.
- **FR-005**: System MUST preserve readable controller and tray status text even if the main status page battery presentation becomes graphical.
- **FR-006**: System MUST display the app version in the bottom-right area of the main window status page.
- **FR-007**: The displayed version MUST use the format `v-<version>`.
- **FR-008**: System MUST derive the displayed version from the same version source used by the build or packaging workflow.
- **FR-009**: System MUST provide a non-empty fallback version when the app is built outside a tagged release flow.
- **FR-010**: System MUST preserve the existing controller input behavior and status refresh behavior outside the new UI presentation changes.
- **FR-011**: System MUST continue to build successfully for Win32 and x64 after the feature is added.
- **FR-012**: If release artifact naming or executable metadata is updated for version alignment, the same version value MUST remain consistent across packaged outputs and in-app display.

### Key Entities *(include if feature involves data)*

- **Build Version Value**: The normalized application version string used for UI display, build metadata, and packaged release alignment.
- **Battery Presentation State**: The structured controller battery information needed by the UI, including connection state, charge availability, charge bucket, charging state, and fallback label.
- **Status Page Footer**: The bottom-right UI region that displays version information without obscuring existing status controls.

## Assumptions

- Existing Git release tags follow the `v<semver>` pattern already present in the repository.
- The battery UI can use coarse charge buckets when the controller API does not expose an exact percentage.
- A local or development build may use a fallback version string when no release version is injected.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: In manual validation, users can identify low, medium, and high battery states from the status page battery bar without reading controller-specific battery text in 100% of tested states.
- **SC-002**: In manual validation, charging state remains visible whenever a supported controller reports charging in 100% of tested charging scenarios.
- **SC-003**: In Win32 and x64 validation builds, the bottom-right footer always shows a non-empty version label and matches the intended build version in 100% of tested launches.
- **SC-004**: Running the existing build and packaging flow with a specified version completes without introducing version mismatches between packaged artifact naming and in-app version display.