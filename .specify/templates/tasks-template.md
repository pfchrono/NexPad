---

description: "Task list template for feature implementation"
---

# Tasks: [FEATURE NAME]

**Input**: Design documents from `/specs/[###-feature-name]/`
**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md, data-model.md, contracts/

**Tests**: The examples below include test tasks. Tests are OPTIONAL - only include them if explicitly requested in the feature specification.

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- **Core application**: `Windows/NexPad/` for native C++ sources, headers, resources, and project files
- **Build and packaging**: `scripts/` plus output validation through `debug/`, `release/`, and `artifacts/` when relevant
- **Documentation and runtime assets**: `README.md`, `RELEASE_CHECKLIST.md`, `Configs/`, `presets/`, and `docs/`
- **Feature artifacts**: `specs/[###-feature-name]/`
- Paths shown below should be adjusted to the actual plan.md structure, but NexPad tasks should default to the Win32 repository layout above

<!-- 
  ============================================================================
  IMPORTANT: The tasks below are SAMPLE TASKS for illustration purposes only.
  
  The /speckit.tasks command MUST replace these with actual tasks based on:
  - User stories from spec.md (with their priorities P1, P2, P3...)
  - Feature requirements from plan.md
  - Entities from data-model.md
  - Endpoints from contracts/
  
  Tasks MUST be organized by user story so each story can be:
  - Implemented independently
  - Tested independently
  - Delivered as an MVP increment
  
  DO NOT keep these sample tasks in the generated tasks.md file.
  ============================================================================
-->

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and basic structure

- [ ] T001 Create or verify feature artifacts under `specs/[###-feature-name]/`
- [ ] T002 Identify impacted files under `Windows/NexPad/`, `scripts/`, `README.md`, and runtime asset locations
- [ ] T003 [P] Capture feature-specific validation commands and manual hardware checks from the plan

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

Examples of foundational tasks (adjust based on your project):

- [ ] T004 Add or adjust shared controller, transport, or configuration state in `Windows/NexPad/*.h`
- [ ] T005 [P] Implement core parsing or config plumbing in `Windows/NexPad/CXBOXController.cpp`, `Windows/NexPad/NexPad.cpp`, or `Windows/NexPad/ConfigFile.cpp`
- [ ] T006 [P] Add safe reset or disconnect handling for any new controller interaction state
- [ ] T007 Define required doc or preset/config updates that every user story depends on
- [ ] T008 Capture required MSBuild validation and any packaging validation gates for the feature

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - [Title] (Priority: P1) 🎯 MVP

**Goal**: [Brief description of what this story delivers]

**Independent Test**: [How to verify this story works on its own]

### Tests for User Story 1 (OPTIONAL - only if tests requested) ⚠️

> **NOTE: Write these tests FIRST, ensure they FAIL before implementation**

- [ ] T010 [P] [US1] Contract test for [endpoint] in tests/contract/test_[name].py
- [ ] T011 [P] [US1] Integration test for [user journey] in tests/integration/test_[name].py

### Implementation for User Story 1

- [ ] T012 [P] [US1] Update shared controller data structures in `Windows/NexPad/*.h`
- [ ] T013 [P] [US1] Implement the primary input behavior in `Windows/NexPad/CXBOXController.cpp` or `Windows/NexPad/NexPad.cpp`
- [ ] T014 [US1] Integrate the behavior into the main NexPad loop or mapping path in `Windows/NexPad/NexPad.cpp`
- [ ] T015 [US1] Add or update config defaults in `Windows/NexPad/ConfigFile.cpp` if the story introduces a user-facing setting
- [ ] T016 [US1] Add validation and fail-safe handling for stale, missing, or conflicting controller input
- [ ] T017 [US1] Document the user-visible behavior in `README.md` if the story changes runtime behavior

**Checkpoint**: At this point, User Story 1 should be fully functional and testable independently

---

## Phase 4: User Story 2 - [Title] (Priority: P2)

**Goal**: [Brief description of what this story delivers]

**Independent Test**: [How to verify this story works on its own]

### Tests for User Story 2 (OPTIONAL - only if tests requested) ⚠️

- [ ] T018 [P] [US2] Contract test for [endpoint] in tests/contract/test_[name].py
- [ ] T019 [P] [US2] Integration test for [user journey] in tests/integration/test_[name].py

### Implementation for User Story 2

- [ ] T020 [P] [US2] Update secondary controller or UI/config paths in `Windows/NexPad/`
- [ ] T021 [US2] Preserve or refine interaction behavior shared with User Story 1 in the relevant Win32 source files
- [ ] T022 [US2] Add or update any settings-tab or preset-facing behavior if required by the story
- [ ] T023 [US2] Verify integration with User Story 1 behavior and unchanged legacy mappings

**Checkpoint**: At this point, User Stories 1 AND 2 should both work independently

---

## Phase 5: User Story 3 - [Title] (Priority: P3)

**Goal**: [Brief description of what this story delivers]

**Independent Test**: [How to verify this story works on its own]

### Tests for User Story 3 (OPTIONAL - only if tests requested) ⚠️

- [ ] T024 [P] [US3] Contract test for [endpoint] in tests/contract/test_[name].py
- [ ] T025 [P] [US3] Integration test for [user journey] in tests/integration/test_[name].py

### Implementation for User Story 3

- [ ] T026 [P] [US3] Implement transport or disconnect safety handling in `Windows/NexPad/CXBOXController.cpp` or `Windows/NexPad/NexPad.cpp`
- [ ] T027 [US3] Add recovery or fallback behavior for limited report modes, stale state, or reconnect flows
- [ ] T028 [US3] Validate unchanged behavior for unaffected controllers and existing mappings

**Checkpoint**: All user stories should now be independently functional

---

[Add more user story phases as needed, following the same pattern]

---

## Phase N: Polish & Cross-Cutting Concerns

**Purpose**: Improvements that affect multiple user stories

- [ ] TXXX [P] Documentation updates in `README.md`, `RELEASE_CHECKLIST.md`, or `docs/`
- [ ] TXXX Code cleanup and refactoring
- [ ] TXXX Performance optimization across all stories
- [ ] TXXX [P] Additional automated tests if the feature explicitly requires them
- [ ] TXXX Security hardening
- [ ] TXXX Run required `Debug|Win32` and `Debug|x64` MSBuild validation
- [ ] TXXX Run packaging validation if runtime assets, output layout, or release behavior changed
- [ ] TXXX Complete manual controller hardware checks if the feature changes input behavior

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phase 3+)**: All depend on Foundational phase completion
  - User stories can then proceed in parallel (if staffed)
  - Or sequentially in priority order (P1 → P2 → P3)
- **Polish (Final Phase)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational (Phase 2) - No dependencies on other stories
- **User Story 2 (P2)**: Can start after Foundational (Phase 2) - May integrate with US1 but should be independently testable
- **User Story 3 (P3)**: Can start after Foundational (Phase 2) - May integrate with US1/US2 but should be independently testable

### Within Each User Story

- Tests (if included) MUST be written and FAIL before implementation
- Shared controller state before behavior integration
- HID or config plumbing before UI or documentation polish
- Core implementation before regression validation
- Story complete before moving to next priority

### Parallel Opportunities

- All Setup tasks marked [P] can run in parallel
- All Foundational tasks marked [P] can run in parallel (within Phase 2)
- Once Foundational phase completes, all user stories can start in parallel (if team capacity allows)
- All tests for a user story marked [P] can run in parallel
- Models within a story marked [P] can run in parallel
- Different user stories can be worked on in parallel by different team members

---

## Parallel Example: User Story 1

```bash
# Launch all tests for User Story 1 together (if tests requested):
Task: "Contract test for [endpoint] in tests/contract/test_[name].py"
Task: "Integration test for [user journey] in tests/integration/test_[name].py"

# Launch all models for User Story 1 together:
Task: "Create [Entity1] model in src/models/[entity1].py"
Task: "Create [Entity2] model in src/models/[entity2].py"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational (CRITICAL - blocks all stories)
3. Complete Phase 3: User Story 1
4. **STOP and VALIDATE**: Test User Story 1 independently
5. Deploy/demo if ready

### Incremental Delivery

1. Complete Setup + Foundational → Foundation ready
2. Add User Story 1 → Test independently → Deploy/Demo (MVP!)
3. Add User Story 2 → Test independently → Deploy/Demo
4. Add User Story 3 → Test independently → Deploy/Demo
5. Each story adds value without breaking previous stories

### Parallel Team Strategy

With multiple developers:

1. Team completes Setup + Foundational together
2. Once Foundational is done:
   - Developer A: User Story 1
   - Developer B: User Story 2
   - Developer C: User Story 3
3. Stories complete and integrate independently

---

## Notes

- [P] tasks = different files, no dependencies
- [Story] label maps task to specific user story for traceability
- Each user story should be independently completable and testable
- Verify tests fail before implementing
- Commit after each task or logical group
- Stop at any checkpoint to validate story independently
- Brownfield NexPad tasks should name actual files under `Windows/NexPad/`, `scripts/`, `README.md`, `Configs/`, or `presets/`
- Controller-affecting features should include both build validation and explicit manual hardware regression checks
- Avoid: vague tasks, same file conflicts, cross-story dependencies that break independence
