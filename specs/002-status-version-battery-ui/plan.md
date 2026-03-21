# Implementation Plan: Status Version And Battery UI

**Branch**: `[002-status-version-battery-ui]` | **Date**: 2026-03-21 | **Spec**: `/specs/002-status-version-battery-ui/spec.md`
**Input**: Feature specification from `/specs/002-status-version-battery-ui/spec.md`

**Note**: This file is the planning artifact for the new NexPad status UI feature.

## Summary

Add two status-surface improvements to NexPad: a bottom-right app version label whose value aligns with the build or release version, and a color-coded battery bar that replaces the current battery text-only status row while preserving charging visibility and text fallbacks where needed. The implementation should expose structured battery presentation data from the controller layer, add a single version source that can be shared by builds and packages, and update the status page layout in `Windows/NexPad/main.cpp` without regressing the existing controller input behavior, tray tooltip behavior, or packaging workflow.

## Technical Context

**Language/Version**: C++ with Visual Studio 2022 `v143` toolset plus PowerShell build and packaging scripts  
**Primary Dependencies**: Win32 UI APIs, XInput, HID/SetupAPI, existing NexPad packaging and status-page UI  
**Storage**: Executable-relative app assets plus build-time version metadata sourced from scripts or generated headers  
**Testing**: MSBuild validation for `Windows/NexPad.sln`, packaging smoke validation through `scripts/package-release.ps1`, and manual status UI checks with connected, disconnected, and charging controllers  
**Target Platform**: Windows 10/11 x32 and x64 desktop environments  
**Project Type**: Native Windows desktop utility  
**Performance Goals**: Status UI updates must remain lightweight and should not add visible lag or interfere with the controller loop  
**Constraints**: Preserve existing controller input behavior, tray tooltip readability, executable-relative runtime layout, and packaging flow while keeping version and battery presentation consistent across debug and release contexts  
**Scale/Scope**: Brownfield enhancement centered on `Windows/NexPad/main.cpp`, `Windows/NexPad/CXBOXController.*`, `Windows/NexPad/NexPad.vcxproj`, `scripts/package-release.ps1`, and possibly resource metadata files if executable version properties are included

## Constitution Check

*GATE: Must pass before implementation. Re-check after design.*

- PASS: The feature remains inside the existing Win32 solution and build pipeline instead of introducing a new subsystem or external updater.
- PASS: Existing controller input behavior is preserved because the requested work changes only status presentation and version plumbing, not the controller mapping loop itself.
- PASS: Validation can use the existing solution builds plus a packaging smoke check and manual UI inspection.
- PASS: The change scope is narrow and targeted to status presentation, structured battery data, and build-version alignment.
- PASS: Any packaging or metadata update can be kept aligned with the existing `Version` parameter already accepted by `scripts/package-release.ps1`.

## Project Structure

### Documentation (this feature)

```text
specs/002-status-version-battery-ui/
├── spec.md
├── plan.md
└── tasks.md
```

### Source Code (repository root)

```text
Windows/
├── NexPad.sln
└── NexPad/
        ├── CXBOXController.h
        ├── CXBOXController.cpp
        ├── main.cpp
        ├── resource.h
        ├── Resource.rc
        └── NexPad.vcxproj

scripts/
└── package-release.ps1

README.md
```

**Structure Decision**: Keep the feature inside the existing native UI and packaging surfaces. Add only the minimum new build-time version plumbing required for a shared app version source.

## Phase 0 Research Outcomes

1. The repository already uses release tags in the form `v0.1.0` through `v0.1.3`, so the requested display format can reuse the existing version scheme.
2. `scripts/package-release.ps1` already accepts a `Version` parameter, making it the most obvious release-version injection point.
3. The app currently exposes battery state to the UI only as text from `CXBOXController`, which is not sufficient for a graphical battery bar without parsing UI strings.
4. The main status page layout in `Windows/NexPad/main.cpp` is simple and can accommodate a footer version label and battery bar with moderate layout adjustments.
5. `Resource.rc` currently lacks executable version metadata, so file properties alignment would require explicit resource additions if kept in scope.

## Phase 1 Design

### Planned Code Changes

- `Windows/NexPad/CXBOXController.h` and `Windows/NexPad/CXBOXController.cpp`: add a structured battery presentation model for the UI while preserving text status access for tray or fallback surfaces.
- `Windows/NexPad/main.cpp`: replace the battery text-only row with a bar-based presentation, show charging state beside it, add a bottom-right version label, and adjust status-page layout and refresh logic.
- `Windows/NexPad/NexPad.vcxproj`: add a shared version value input to the native build if needed, likely through preprocessor definitions or a generated include.
- `scripts/package-release.ps1`: align the package version input with the same version source used by the app display.
- `Windows/NexPad/resource.h` and `Windows/NexPad/Resource.rc`: update only if executable file version metadata is included as part of the shared version system.

### Validation Strategy

- Build validation: run MSBuild for `Debug|Win32` and `Debug|x64` on `Windows/NexPad.sln`.
- Packaging validation: run a packaging smoke test with an explicit version value through `scripts/package-release.ps1` and confirm artifact version naming remains aligned.
- Manual UI validation: verify disconnected, wired, charging, low, medium, high, and unknown battery states on the main status page.
- Version validation: confirm the bottom-right footer shows the expected `v-<version>` label in debug and packaged builds.

### Risks And Mitigations

- Risk: UI code starts parsing battery text to drive the bar, making the display brittle.
  Mitigation: expose structured battery presentation data from the controller layer rather than reverse-parsing UI strings.
- Risk: Build and package version values drift apart.
  Mitigation: use one shared version source or generated value for both artifact naming and in-app display.
- Risk: The battery bar looks correct for one controller path but misrepresents XInput wired or unknown states.
  Mitigation: define explicit fallback states for wired, disconnected, and unavailable battery data before UI rendering.
- Risk: The new footer and battery bar crowd the status page.
  Mitigation: update layout and redraw logic together rather than trying to squeeze new text into the old battery row.

## Post-Design Constitution Re-check

- PASS: Design still uses the existing Win32 app structure and packaging flow.
- PASS: No existing controller input mapping behavior needs to change to deliver the requested UI/status feature.
- PASS: Validation remains grounded in the current solution build plus a packaging smoke check.
- PASS: The feature remains small, local, and reversible if needed.

## Complexity Tracking

No constitution violations or unjustified complexity were identified. The only meaningful decision is whether executable file version metadata should be updated in addition to in-app version display; that can remain scoped to the same shared version source if implemented.