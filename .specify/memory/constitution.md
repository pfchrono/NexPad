<!--
Sync Impact Report
Version change: template -> 1.0.0
Modified principles:
- Added I. Native Windows Integrity
- Added II. Safe Controller Semantics
- Added III. Verifiable Build and Release Outputs
- Added IV. Focused Brownfield Changes
- Added V. Docs and Runtime Configuration Stay Aligned
Added sections:
- Platform and Delivery Constraints
- Development Workflow and Quality Gates
Removed sections:
- Placeholder template sections and example comments
Templates requiring updates:
- ✅ updated: .specify/templates/plan-template.md
- ⚠ pending: .specify/templates/spec-template.md
- ⚠ pending: .specify/templates/tasks-template.md
- ✅ updated: README.md
Follow-up TODOs:
- None
-->

# NexPad Constitution

## Core Principles

### I. Native Windows Integrity
NexPad MUST remain a native Windows desktop utility built around the existing
Visual Studio solution, Win32 UI, and controller input pipeline. Feature work
MUST preserve the current repository structure centered on `Windows/NexPad.sln`,
the `Windows/NexPad/` sources, and the executable-relative runtime asset model
for `config.ini` and `presets/`. New work MUST not introduce heavyweight runtime
frameworks or cross-platform abstractions unless the change is explicitly scoped
as an architectural migration.

### II. Safe Controller Semantics
Controller input changes MUST prioritize correctness and recovery over feature
breadth. Any change that touches XInput, HID parsing, touchpad behavior,
 trigger mapping, or mouse/key emission MUST preserve safe release behavior,
 avoid stale or phantom inputs, and consider disconnect or reconnect flows.
 Changes that affect controller semantics MUST include regression checks for the
 existing supported paths they could impact, especially XInput, USB DualSense,
 and Bluetooth DualSense when relevant.

### III. Verifiable Build and Release Outputs
Every meaningful implementation MUST be validated against the real build system.
 At minimum, changes MUST pass the applicable MSBuild targets for the affected
 configurations, and packaging or release changes MUST preserve the documented
 output layout under `debug/x32`, `debug/x64`, `release/x32`, and `release/x64`.
 Release-facing changes MUST keep `README.md`, `RELEASE_CHECKLIST.md`, and the
 packaging scripts consistent with actual outputs and runtime requirements.

### IV. Focused Brownfield Changes
NexPad is a brownfield project. Work MUST solve the root cause of the requested
 problem with the smallest defensible change set and MUST avoid unrelated
 refactors, formatting churn, or speculative subsystem rewrites. New patterns
 SHOULD follow the surrounding code style and existing public behavior unless a
 behavior change is explicitly intended and documented.

### V. Docs and Runtime Configuration Stay Aligned
User-visible behavior, generated defaults, Settings UI, and runtime docs MUST
 stay consistent. If a feature is configurable, its config keys, UI controls,
 defaults, and README guidance MUST describe the same behavior. If a workflow is
 added to the repository, contributors MUST be able to discover how to use it
 from repository documentation without reverse-engineering generated files.

## Platform and Delivery Constraints

- Target platform MUST remain Windows 10/11 with the Visual Studio 2022 `v143`
	toolset.
- Primary implementation language is native C++ in the existing Win32 project.
- Runtime assets MUST be resolved relative to the executable directory.
- Build and packaging scripts MUST stay PowerShell-friendly on Windows.
- Hardware-dependent controller behavior MAY require manual validation in
	addition to successful builds; plans and tasks MUST call this out explicitly
	when applicable.

## Development Workflow and Quality Gates

- Spec-driven work SHOULD begin with a constitution-aware feature spec, then an
	implementation plan, then task breakdown, before code execution for larger
	changes.
- Plans for NexPad MUST name the actual repository paths they will touch and
	MUST include a controller regression strategy when input behavior is changed.
- Before completing a feature, contributors MUST run the relevant MSBuild
	validation steps and document any manual hardware checks that remain.
- Release and packaging changes MUST be verified with the real scripts, not only
	by static inspection.
- Documentation updates are required when behavior, config keys, build outputs,
	or contributor workflow materially change.

## Governance

This constitution governs spec, plan, task, and implementation artifacts for
NexPad. All future Spec Kit outputs MUST satisfy these principles before work is
considered ready. Amendments require:

- an explicit update to this file,
- a semantic version decision recorded in the Sync Impact Report,
- review of any affected templates or contributor-facing docs, and
- a migration note when the amendment changes expected workflow or quality gates.

Compliance review MUST confirm controller safety, build validation, and doc or
config alignment where applicable. Repository guidance in `README.md` and
`RELEASE_CHECKLIST.md` remains the operational reference for contributors and
release work.

**Version**: 1.0.0 | **Ratified**: 2026-03-21 | **Last Amended**: 2026-03-21
