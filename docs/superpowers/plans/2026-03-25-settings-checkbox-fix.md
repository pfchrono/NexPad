# Settings Checkbox Immediate-Apply Fix Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the three Settings tab checkboxes (Enable DualSense Touchpad, Swap Thumbsticks, Start NexPad with Windows) apply immediately when clicked, and show live state on the Status tab.

**Architecture:** All changes are in a single file (`Windows/NexPad/main.cpp`). The fix adds NexPad setter calls directly to each checkbox's existing `BN_CLICKED` handler, and extends the Status tab with three new `STATIC` labels driven by `updateStatusControls()`.

**Tech Stack:** Win32 C++, MSVC v143, MSBuild

---

## File Map

| File | Changes |
|------|---------|
| `Windows/NexPad/main.cpp` | IDC enum, GuiState fields, control creation, layout, updateStatusControls, checkbox handlers |

---

## Spec Reference

`docs/superpowers/specs/2026-03-25-settings-checkbox-fix-design.md`

---

### Task 1: Add IDC constants and GuiState fields

**Files:**
- Modify: `Windows/NexPad/main.cpp:91` (IDC enum)
- Modify: `Windows/NexPad/main.cpp:122` (GuiState HWND fields)
- Modify: `Windows/NexPad/main.cpp` (GuiState cached string fields)

**Context:** The anonymous enum in `main.cpp` (around line 60–92) defines Win32 control IDs. `IDC_SETTINGS_NOTE` is currently the last entry with no trailing comma. `GuiState` (line 99+) holds all window handles and cached display strings.

- [ ] **Step 1: Add IDC constants to the enum**

In `main.cpp`, find the enum ending with:
```cpp
    IDC_SETTINGS_NOTE
  };
```
Change it to:
```cpp
    IDC_SETTINGS_NOTE,
    IDC_TOUCHPAD_STATUS_TEXT,
    IDC_SWAP_STATUS_TEXT,
    IDC_START_WITH_WINDOWS_STATUS_TEXT,
  };
```

- [ ] **Step 2: Add HWND fields to GuiState**

In `GuiState`, find:
```cpp
    HWND configText = NULL;
    HWND versionText = NULL;
```
Change to:
```cpp
    HWND configText = NULL;
    HWND touchpadStatusText = NULL;
    HWND swapStatusText = NULL;
    HWND startWithWindowsStatusText = NULL;
    HWND versionText = NULL;
```

- [ ] **Step 3: Add cached string fields to GuiState**

In `GuiState`, find the cached string block. Look for `cachedConfigText` (search for it). After that line add:
```cpp
    std::string cachedTouchpadStatusText;
    std::string cachedSwapStatusText;
    std::string cachedStartWithWindowsStatusText;
```

- [ ] **Step 4: Build to verify no compile errors**

```powershell
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64 /t:NexPad /m
```
Expected: Build succeeds. New fields are unused for now — that's fine.

- [ ] **Step 5: Commit**

```bash
git add Windows/NexPad/main.cpp
git commit -m "feat: add IDC constants and GuiState fields for settings status labels"
```

---

### Task 2: Create the three STATIC controls on the status page

**Files:**
- Modify: `Windows/NexPad/main.cpp:~2496` (WM_CREATE control creation block)
- Modify: `Windows/NexPad/main.cpp:~2580` (setControlFont list)

**Context:** Status page controls are created around lines 2482–2505 in the `WM_CREATE` handler. All controls start with zero position/size — they're positioned later by `MoveWindow` in a resize handler. The `setControlFont` call (line ~2580) assigns the UI font to every control via a brace-enclosed initializer list.

- [ ] **Step 1: Add CreateWindowExA calls after configText creation**

Find:
```cpp
      state->configText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
                                          0, 0, 0, 0, state->statusPage, reinterpret_cast<HMENU>(IDC_CONFIG_TEXT), createStruct->hInstance, NULL);
      state->versionText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE | SS_RIGHT,
```
Insert between them:
```cpp
      state->touchpadStatusText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
                                                  0, 0, 0, 0, state->statusPage, reinterpret_cast<HMENU>(IDC_TOUCHPAD_STATUS_TEXT), createStruct->hInstance, NULL);
      state->swapStatusText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
                                              0, 0, 0, 0, state->statusPage, reinterpret_cast<HMENU>(IDC_SWAP_STATUS_TEXT), createStruct->hInstance, NULL);
      state->startWithWindowsStatusText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
                                                          0, 0, 0, 0, state->statusPage, reinterpret_cast<HMENU>(IDC_START_WITH_WINDOWS_STATUS_TEXT), createStruct->hInstance, NULL);
```

- [ ] **Step 2: Add the three HWNDs to the setControlFont list**

Find the `setControlFont` call (line ~2580). Its first line in the brace list is:
```cpp
                     {state->tab, state->statusText, state->controllerText, state->controllerTypeText, state->batteryText, state->speedText, state->scrollText, state->configText, state->versionText,
```
Change `state->configText, state->versionText,` to `state->configText, state->touchpadStatusText, state->swapStatusText, state->startWithWindowsStatusText, state->versionText,`

- [ ] **Step 3: Build to verify no compile errors**

```powershell
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64 /t:NexPad /m
```
Expected: Build succeeds.

- [ ] **Step 4: Commit**

```bash
git add Windows/NexPad/main.cpp
git commit -m "feat: create status tab STATIC controls for settings state labels"
```

---

### Task 3: Add MoveWindow layout calls and push textTop down

**Files:**
- Modify: `Windows/NexPad/main.cpp:~2067` (status page layout / resize handler)

**Context:** The status page layout happens in a resize/layout function around lines 2050–2073. `configText` is placed at `margin + 168` (line 2067). `textTop` (the y-position where the text edit areas start) is defined as `margin + 200` (line 2069). Without `MoveWindow` calls, the new labels remain at 0,0,0,0 and are invisible.

- [ ] **Step 1: Find the layout block and add MoveWindow calls**

Find (around line 2067):
```cpp
    MoveWindow(state->configText, margin, margin + 168, pageWidth - margin * 2, labelHeight, TRUE);
    const int textTop = margin + 200;
```
Replace with:
```cpp
    MoveWindow(state->configText, margin, margin + 168, pageWidth - margin * 2, labelHeight, TRUE);
    MoveWindow(state->touchpadStatusText, margin, margin + 196, pageWidth - margin * 2, labelHeight, TRUE);
    MoveWindow(state->swapStatusText, margin, margin + 224, pageWidth - margin * 2, labelHeight, TRUE);
    MoveWindow(state->startWithWindowsStatusText, margin, margin + 252, pageWidth - margin * 2, labelHeight, TRUE);
    const int textTop = margin + 284;
```

- [ ] **Step 2: Build to verify no compile errors**

```powershell
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64 /t:NexPad /m
```
Expected: Build succeeds.

- [ ] **Step 3: Commit**

```bash
git add Windows/NexPad/main.cpp
git commit -m "feat: add MoveWindow layout for settings status labels, push textTop down"
```

---

### Task 4: Populate labels in updateStatusControls()

**Files:**
- Modify: `Windows/NexPad/main.cpp:~1848` (updateStatusControls)

**Context:** `updateStatusControls()` starts at line 1811. It calls `setTextIfChanged(hwnd, cachedString, newValue)` for each label — this helper only calls `SetWindowTextA` when the string actually changes, avoiding flicker. `configText` is updated at line 1848. Getter signatures: `getTouchpadEnabled()` → `int`, `getSwapThumbsticks()` → `int`, `getStartWithWindows()` → `int`. Non-zero = enabled.

- [ ] **Step 1: Add three setTextIfChanged calls after configText**

Find (around line 1848):
```cpp
    setTextIfChanged(state->configText, state->cachedConfigText, ...);
```
(the exact string argument doesn't matter — find the `configText` call). Immediately after it insert:
```cpp
    setTextIfChanged(state->touchpadStatusText, state->cachedTouchpadStatusText,
                     std::string("Touchpad: ") + (state->nexPad.getTouchpadEnabled() ? "Enabled" : "Disabled"));
    setTextIfChanged(state->swapStatusText, state->cachedSwapStatusText,
                     std::string("Swap thumbsticks: ") + (state->nexPad.getSwapThumbsticks() ? "On" : "Off"));
    setTextIfChanged(state->startWithWindowsStatusText, state->cachedStartWithWindowsStatusText,
                     std::string("Start with Windows: ") + (state->nexPad.getStartWithWindows() ? "On" : "Off"));
```

- [ ] **Step 2: Build to verify no compile errors**

```powershell
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64 /t:NexPad /m
```
Expected: Build succeeds.

- [ ] **Step 3: Manual smoke test — status labels visible**

Run `release\x64\NexPad.exe`. Open Status tab. Verify three new lines appear showing the current config values (e.g. "Touchpad: Disabled", "Swap thumbsticks: Off", "Start with Windows: Off"). Labels should be positioned below the Config line with no overlap.

- [ ] **Step 4: Commit**

```bash
git add Windows/NexPad/main.cpp
git commit -m "feat: populate settings state labels in updateStatusControls"
```

---

### Task 5: Extend IDC_TOUCHPAD_CHECK and IDC_SWAP_CHECK handlers

**Files:**
- Modify: `Windows/NexPad/main.cpp:~2743` (WM_COMMAND checkbox handlers)

**Context:** The checkbox `BN_CLICKED` handlers are in the `WM_COMMAND` block around lines 2743–2768. Each handler reads the current check state, inverts it, calls `BM_SETCHECK`, and calls `InvalidateRect`. The local variable `checked` holds the **new** post-toggle state (BST_CHECKED = 1 = enabled, BST_UNCHECKED = 0 = disabled). `saveConfigFile()` persists all in-memory settings to `config.ini`. `appendOutput()` appends a timestamped line to the output log on the Status tab.

- [ ] **Step 1: Extend IDC_TOUCHPAD_CHECK handler**

Find the handler (the one with `state->touchpadCheck`):
```cpp
      case IDC_TOUCHPAD_CHECK:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          const LRESULT checked = SendMessage(state->touchpadCheck, BM_GETCHECK, 0, 0) == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED;
          SendMessage(state->touchpadCheck, BM_SETCHECK, checked, 0);
          InvalidateRect(state->touchpadCheck, NULL, TRUE);
        }
        return 0;
```
Replace with:
```cpp
      case IDC_TOUCHPAD_CHECK:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          const LRESULT checked = SendMessage(state->touchpadCheck, BM_GETCHECK, 0, 0) == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED;
          SendMessage(state->touchpadCheck, BM_SETCHECK, checked, 0);
          InvalidateRect(state->touchpadCheck, NULL, TRUE);
          state->nexPad.setTouchpadEnabled(checked == BST_CHECKED ? 1 : 0);
          state->nexPad.saveConfigFile();
          updateStatusControls(window);
          appendOutput(window, checked == BST_CHECKED ? "Touchpad enabled." : "Touchpad disabled.");
        }
        return 0;
```

- [ ] **Step 2: Extend IDC_SWAP_CHECK handler**

Find the handler (the one with `state->swapCheck`):
```cpp
      case IDC_SWAP_CHECK:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          const LRESULT checked = SendMessage(state->swapCheck, BM_GETCHECK, 0, 0) == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED;
          SendMessage(state->swapCheck, BM_SETCHECK, checked, 0);
          InvalidateRect(state->swapCheck, NULL, TRUE);
        }
        return 0;
```
Replace with:
```cpp
      case IDC_SWAP_CHECK:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          const LRESULT checked = SendMessage(state->swapCheck, BM_GETCHECK, 0, 0) == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED;
          SendMessage(state->swapCheck, BM_SETCHECK, checked, 0);
          InvalidateRect(state->swapCheck, NULL, TRUE);
          state->nexPad.setSwapThumbsticks(checked == BST_CHECKED ? 1 : 0);
          state->nexPad.saveConfigFile();
          updateStatusControls(window);
          appendOutput(window, checked == BST_CHECKED ? "Swap thumbsticks: on." : "Swap thumbsticks: off.");
        }
        return 0;
```

- [ ] **Step 3: Build to verify no compile errors**

```powershell
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64 /t:NexPad /m
```
Expected: Build succeeds.

- [ ] **Step 4: Manual smoke test — touchpad and swap apply immediately**

Run `release\x64\NexPad.exe`. Go to Settings tab. Click "Enable DualSense Touchpad" — no Apply needed. Switch to Status tab. Verify "Touchpad: Enabled" appears. Output log shows "Touchpad enabled." Repeat for Swap Thumbsticks. Uncheck each — verify status label reverts and log shows the "off" message.

- [ ] **Step 5: Commit**

```bash
git add Windows/NexPad/main.cpp
git commit -m "feat: apply touchpad and swap thumbsticks settings immediately on checkbox click"
```

---

### Task 6: Extend IDC_START_WITH_WINDOWS_CHECK handler

**Files:**
- Modify: `Windows/NexPad/main.cpp:~2752` (WM_COMMAND Start with Windows handler)

**Context:** `setStartWithWindows(int desired, std::string& errorOut)` returns `bool` — `false` when the registry write fails (e.g. running without admin rights). On failure the checkbox must revert its visual state. This matches the pattern already used in `applySettings()` (lines 2190–2203). The existing handler for this checkbox is `IDC_START_WITH_WINDOWS_CHECK` around line 2752.

- [ ] **Step 1: Replace IDC_START_WITH_WINDOWS_CHECK handler**

Find:
```cpp
      case IDC_START_WITH_WINDOWS_CHECK:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          const LRESULT checked = SendMessage(state->startWithWindowsCheck, BM_GETCHECK, 0, 0) == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED;
          SendMessage(state->startWithWindowsCheck, BM_SETCHECK, checked, 0);
          InvalidateRect(state->startWithWindowsCheck, NULL, TRUE);
        }
        return 0;
```
Replace with:
```cpp
      case IDC_START_WITH_WINDOWS_CHECK:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          const LRESULT checked = SendMessage(state->startWithWindowsCheck, BM_GETCHECK, 0, 0) == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED;
          SendMessage(state->startWithWindowsCheck, BM_SETCHECK, checked, 0);
          InvalidateRect(state->startWithWindowsCheck, NULL, TRUE);
          std::string startupError;
          const bool ok = state->nexPad.setStartWithWindows(checked == BST_CHECKED ? 1 : 0, startupError);
          if (!ok)
          {
            SendMessage(state->startWithWindowsCheck, BM_SETCHECK,
                        checked == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED, 0);
            InvalidateRect(state->startWithWindowsCheck, NULL, TRUE);
            const std::string msg = std::string("Unable to update Start with Windows: ") + startupError;
            appendOutput(window, msg);
            MessageBoxA(window, msg.c_str(), "NexPad", MB_OK | MB_ICONERROR);
          }
          else
          {
            state->nexPad.saveConfigFile();
            updateStatusControls(window);
            appendOutput(window, checked == BST_CHECKED ? "Start with Windows: on." : "Start with Windows: off.");
          }
        }
        return 0;
```

- [ ] **Step 2: Build all configurations**

```powershell
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=Win32 /t:NexPad /m
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64 /t:NexPad /m
msbuild Windows/NexPad.sln /p:Configuration=Debug /p:Platform=x64 /t:NexPad /m
```
Expected: All succeed.

- [ ] **Step 3: Manual smoke test — happy path**

Run `release\x64\NexPad.exe` as Administrator. Click "Start NexPad with Windows". Status tab shows "Start with Windows: On". Output log shows "Start with Windows: on." Uncheck — reverts. Check registry `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` to confirm `NexPad` key added/removed.

- [ ] **Step 4: Manual smoke test — error path**

Run `release\x64\NexPad.exe` **without** admin rights on a system where registry write will fail (or temporarily make `setStartWithWindows` return false to simulate). Click the checkbox — verify the checkbox reverts, a MessageBox appears with the error, and the output log contains the error message.

- [ ] **Step 5: Manual smoke test — Apply button still works**

Enable a checkbox, then click Apply. Verify no crash, no double-apply side effects. Output log shows "Applied and auto-saved live settings."

- [ ] **Step 6: Commit**

```bash
git add Windows/NexPad/main.cpp
git commit -m "feat: apply Start with Windows setting immediately on checkbox click with error revert"
```

---

### Task 7: Final build validation

**Files:** No code changes — build + test only.

- [ ] **Step 1: Clean build all configurations**

```powershell
.\scripts\build-all.ps1 -Clean
```
Expected: All four configurations (Debug/Release × Win32/x64) succeed with 0 errors.

- [ ] **Step 2: Run unit tests**

```powershell
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64 /t:NexPadTests /m
.\release\x64\NexPadTests.exe
```
Expected: All 21 tests pass (unit tests don't cover Win32 UI, so no new tests needed here).

- [ ] **Step 3: Commit if any build fixes were needed, otherwise done**

```bash
git log --oneline -8
```
Verify the feature commits are all present. No additional commit needed unless Task 7 uncovered issues.
