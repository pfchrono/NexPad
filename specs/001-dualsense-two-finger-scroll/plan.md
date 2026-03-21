# Implementation Plan: DualSense Two-Finger Scrolling

**Branch**: `[001-dualsense-two-finger-scroll]` | **Date**: 2026-03-21 | **Spec**: `/specs/001-dualsense-two-finger-scroll/spec.md`
**Input**: Feature specification from `/specs/001-dualsense-two-finger-scroll/spec.md`

**Note**: This file is the `/speckit.plan` output for the existing brownfield NexPad repository.

## Summary

Add DualSense two-finger scrolling by extending the existing HID touch parsing in `Windows/NexPad/CXBOXController.cpp` to expose reliable multi-touch state and transport capability, then adding touch interaction arbitration in `Windows/NexPad/NexPad.cpp` so one-finger cursor movement, tap-to-click, and two-finger scroll can coexist safely. The initial design keeps the current `TOUCHPAD_ENABLED`, `TOUCHPAD_DEAD_ZONE`, `TOUCHPAD_SPEED`, and `SCROLL_SPEED` surfaces instead of adding a new user-facing toggle, and it preserves the existing fail-safe behavior when Bluetooth falls back to limited reports.

## Technical Context

**Language/Version**: C++ with Visual Studio 2022 `v143` toolset  
**Primary Dependencies**: Win32 `SendInput`, XInput, HID/SetupAPI, existing NexPad native settings and config pipeline  
**Storage**: Executable-relative `config.ini`, `presets/`, generated debug and release asset layouts  
**Testing**: MSBuild validation of `Windows/NexPad.sln` plus manual DualSense USB and Bluetooth gesture regression checks  
**Target Platform**: Windows 10/11 x32 and x64 desktop environments  
**Project Type**: Native Windows desktop utility  
**Performance Goals**: Low-latency controller-to-input behavior suitable for desktop navigation without adding visible lag or stale wheel output  
**Constraints**: Preserve executable-relative runtime assets, current output layout, existing stick scroll semantics, one-finger touchpad cursor/tap behavior, and safe controller release behavior across disconnect and transport changes  
**Scale/Scope**: Brownfield enhancement within the existing single-solution Win32 repository, centered on `Windows/NexPad/` plus aligned docs and config text if user-visible wording changes

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-checked after Phase 1 design.*

- PASS: The design stays inside the existing native Windows solution and executable-relative asset model. Planned implementation remains in `Windows/NexPad/` with only targeted doc/config wording updates at the repository root if behavior descriptions change.
- PASS: Controller safety is a first-class design constraint. The plan covers USB, Bluetooth enhanced reports, Bluetooth limited-report fallback, disconnect, reconnect, stale-report timeout, finger-count transitions, and non-DualSense no-op behavior.
- PASS: Validation uses the real solution build (`Windows/NexPad.sln`) and explicit hardware regression checks. Packaging verification is not required unless implementation changes runtime assets or release layout.
- PASS: The change is narrowly scoped to DualSense touchpad parsing, gesture arbitration, and alignment of any affected user-facing text. No refactor of unrelated input or UI systems is proposed.
- PASS: Phase 1 design keeps the existing touchpad setting surface. If implementation later proves a new user-facing setting is necessary, config generation, config load/save, preset import/export, Settings UI, and README or release notes must be updated together.

## Project Structure

### Documentation (this feature)

```text
specs/001-dualsense-two-finger-scroll/
├── plan.md
├── research.md
├── data-model.md
├── quickstart.md
├── contracts/
│   └── dualsense-touchpad-interaction.md
└── tasks.md
```

### Source Code (repository root)

```text
Windows/
├── NexPad.sln
└── NexPad/
        ├── CXBOXController.h
        ├── CXBOXController.cpp
        ├── NexPad.h
        ├── NexPad.cpp
        ├── main.cpp
        ├── ConfigFile.cpp
        └── *.rc / *.vcxproj / supporting headers

Configs/
├── config_default.ini

docs/
└── releases/

scripts/
├── build-all.ps1
├── build-all.bat
└── package-release.ps1

README.md
config.ini
presets/
RELEASE_CHECKLIST.md
```

**Structure Decision**: Use the existing single-solution Win32 repository layout. The feature should land in the current controller parsing and NexPad loop layers rather than introducing a new subsystem. If wording changes are needed, update the existing root-level docs and config assets instead of adding parallel sources of truth.

## Phase 0 Research Outcomes

1. The controller layer must expose more than the current one-finger `TouchpadState`; otherwise `NexPad` cannot distinguish one-finger cursor motion from intentional two-finger scroll.
2. Gesture arbitration belongs in `NexPad.cpp`, where cursor motion, wheel emission, and tap-to-click already coexist in the main loop. Parsing-only code should stay focused on reliable state extraction and safe resets.
3. Bluetooth support should remain capability-driven. Enhanced reports may participate in two-finger scrolling, while limited Bluetooth reports must continue to fail safe without synthesizing scroll from incomplete touch data.
4. The initial design should avoid a new user-facing toggle. Existing touchpad settings already gate touch behavior across config generation, config persistence, presets, and the Settings tab.
5. Touchpad scroll tuning should preserve the meaning of current settings: `TOUCHPAD_SPEED` remains cursor-only, while touchpad scroll uses internal normalization tied to the existing `SCROLL_SPEED` behavior rather than repurposing config keys.

## Phase 1 Design

### Planned Code Changes

- `Windows/NexPad/CXBOXController.h` and `Windows/NexPad/CXBOXController.cpp`: extend touch state to represent two contacts, active finger count, and whether the current report path exposes reliable two-touch detail. Preserve reset behavior on disconnect, stale reports, and simple Bluetooth fallback.
- `Windows/NexPad/NexPad.h` and `Windows/NexPad/NexPad.cpp`: add a small touch interaction mode state machine so one-finger cursor movement and tap-to-click remain intact while two-finger scroll suppresses pointer movement and tap output. Stop scroll immediately on finger release, transport downgrade, touch disable, or disconnect.
- `Windows/NexPad/main.cpp`: only adjust text if the Settings tab or status output needs clearer wording for additive one-finger cursor plus two-finger scroll behavior.
- `Windows/NexPad/ConfigFile.cpp`, `Configs/config_default.ini`, `config.ini`, `README.md`, and `docs/releases/*`: update only if behavior text or settings descriptions need alignment. The current design assumes wording updates, not a new config key.

### Validation Strategy

- Build validation: run MSBuild for at least `Debug|Win32` and `Debug|x64` on `Windows/NexPad.sln`.
- Manual controller validation: verify one-finger cursor movement, tap-to-click, stick scroll, swap-thumbsticks behavior, two-finger vertical scroll, two-finger horizontal scroll, and non-DualSense controller behavior.
- Safety validation: verify clean stop on finger-count changes, disconnect, reconnect, sleep or wake, and Bluetooth limited-report fallback.
- Documentation and config validation: confirm README and config wording still match behavior if any user-facing text changes.

### Risks And Mitigations

- Risk: A two-finger gesture also moves the cursor or triggers a tap click.
    Mitigation: introduce explicit interaction-mode arbitration and tap suppression whenever two reliable touches are active.
- Risk: Bluetooth fallback reports create phantom scroll or stale state.
    Mitigation: require reliable two-touch capability before entering scroll mode and clear touch-derived state whenever reports are stale, incomplete, or downgraded.
- Risk: Touch deltas produce scroll output that feels disproportionately fast or slow.
    Mitigation: normalize touch-derived wheel output internally while keeping the existing `SCROLL_SPEED` setting semantics consistent.

## Post-Design Constitution Re-check

- PASS: Design still preserves the native Win32 solution structure and executable-relative asset model.
- PASS: Safety coverage explicitly includes USB, Bluetooth enhanced reports, Bluetooth simple fallback, disconnect, reconnect, and stale report handling.
- PASS: Validation remains grounded in real MSBuild and manual hardware checks.
- PASS: No unnecessary refactor or additional subsystem is introduced.
- PASS: No unresolved documentation or config alignment gap remains in the design. If implementation adds a new setting after all, that becomes a blocking alignment task rather than an optional follow-up.

## Complexity Tracking

No constitution violations or justified complexity exceptions were identified during planning.
