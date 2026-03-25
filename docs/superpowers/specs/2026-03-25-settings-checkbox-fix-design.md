# Settings Checkbox Immediate-Apply Fix — Design

## Problem

Three checkboxes on the Settings tab — **Enable DualSense Touchpad**, **Swap Thumbsticks**, and **Start NexPad with Windows** — do not apply when clicked. Their `BN_CLICKED` handlers only toggle the visual check state; no NexPad setter is called and no config is saved until the user clicks Apply. Additionally, the Status tab shows no indication of these settings states even after Apply.

## Root Cause

Each handler (e.g. `IDC_SWAP_CHECK`) calls `BM_SETCHECK` and `InvalidateRect` but nothing else. `applySettings()` is the only place that calls `setSwapThumbsticks`, `setTouchpadEnabled`, and `setStartWithWindows`.

## Solution

Two independent changes:

1. **Immediate-apply on click** — extend each checkbox `BN_CLICKED` handler to call the appropriate NexPad setter, save config, update status display, and append a log message.
2. **Status tab labels** — add three new `STATIC` labels to the Status tab showing live state of these three settings, updated by `updateStatusControls()`.

---

## Change 1: Checkbox handlers (`main.cpp`)

### `IDC_TOUCHPAD_CHECK`

After the existing visual toggle (keep as-is):

```cpp
state->nexPad.setTouchpadEnabled(checked == BST_CHECKED ? 1 : 0);
state->nexPad.saveConfigFile();
updateStatusControls(window);
appendOutput(window, checked == BST_CHECKED ? "Touchpad enabled." : "Touchpad disabled.");
```

### `IDC_SWAP_CHECK`

After the existing visual toggle (keep as-is):

```cpp
state->nexPad.setSwapThumbsticks(checked == BST_CHECKED ? 1 : 0);
state->nexPad.saveConfigFile();
updateStatusControls(window);
appendOutput(window, checked == BST_CHECKED ? "Swap thumbsticks: on." : "Swap thumbsticks: off.");
```

### `IDC_START_WITH_WINDOWS_CHECK`

After the existing visual toggle (keep as-is):

```cpp
std::string startupError;
const bool ok = state->nexPad.setStartWithWindows(checked == BST_CHECKED ? 1 : 0, startupError);
if (!ok)
{
    // Revert checkbox
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
```

### `applySettings()` — no change needed

`applySettings()` already reads these checkboxes and calls the setters. This remains correct and idempotent when the user also clicks Apply.

---

## Change 2: Status tab labels (`main.cpp`)

### IDC enum — add three constants (after `IDC_SETTINGS_NOTE`)

```cpp
IDC_TOUCHPAD_STATUS_TEXT,
IDC_SWAP_STATUS_TEXT,
IDC_START_WITH_WINDOWS_STATUS_TEXT,
```

### `GuiState` — add HWND fields (after `configText`)

```cpp
HWND touchpadStatusText = NULL;
HWND swapStatusText = NULL;
HWND startWithWindowsStatusText = NULL;
```

### `GuiState` — add cached string fields (after `cachedConfigText`)

```cpp
std::string cachedTouchpadStatusText;
std::string cachedSwapStatusText;
std::string cachedStartWithWindowsStatusText;
```

### Status page creation — add three STATIC controls (after `configText` creation at ~line 2496)

```cpp
state->touchpadStatusText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
    0, 0, 0, 0, state->statusPage,
    reinterpret_cast<HMENU>(IDC_TOUCHPAD_STATUS_TEXT), createStruct->hInstance, NULL);
state->swapStatusText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
    0, 0, 0, 0, state->statusPage,
    reinterpret_cast<HMENU>(IDC_SWAP_STATUS_TEXT), createStruct->hInstance, NULL);
state->startWithWindowsStatusText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
    0, 0, 0, 0, state->statusPage,
    reinterpret_cast<HMENU>(IDC_START_WITH_WINDOWS_STATUS_TEXT), createStruct->hInstance, NULL);
```

### Status page layout — add MoveWindow calls and push `textTop` down

Current layout has `configText` at `margin + 168` and `textTop` at `margin + 200`.

New labels go at `margin + 196`, `margin + 224`, `margin + 252` (28 px increments, same as existing rows). `textTop` moves from `margin + 200` to `margin + 284`.

```cpp
MoveWindow(state->configText, margin, margin + 168, pageWidth - margin * 2, labelHeight, TRUE);
MoveWindow(state->touchpadStatusText, margin, margin + 196, pageWidth - margin * 2, labelHeight, TRUE);
MoveWindow(state->swapStatusText, margin, margin + 224, pageWidth - margin * 2, labelHeight, TRUE);
MoveWindow(state->startWithWindowsStatusText, margin, margin + 252, pageWidth - margin * 2, labelHeight, TRUE);

const int textTop = margin + 284;  // was margin + 200
```

### `setControlFont` call (~line 2580) — add three new HWNDs

Add `state->touchpadStatusText`, `state->swapStatusText`, `state->startWithWindowsStatusText` to the existing brace-enclosed list.

### `updateStatusControls()` — add three `setTextIfChanged` calls (after `configText` line)

```cpp
setTextIfChanged(state->touchpadStatusText, state->cachedTouchpadStatusText,
    std::string("Touchpad: ") + (state->nexPad.getTouchpadEnabled() ? "Enabled" : "Disabled"));
setTextIfChanged(state->swapStatusText, state->cachedSwapStatusText,
    std::string("Swap thumbsticks: ") + (state->nexPad.getSwapThumbsticks() ? "On" : "Off"));
setTextIfChanged(state->startWithWindowsStatusText, state->cachedStartWithWindowsStatusText,
    std::string("Start with Windows: ") + (state->nexPad.getStartWithWindows() ? "On" : "Off"));
```

---

## Files Changed

- `Windows/NexPad/main.cpp` — all changes above (single file)

## Testing

- Build with MSBuild (Win32 + x64, Debug + Release)
- Click each checkbox: setting takes effect immediately, status tab updates, log line appears
- Uncheck: reverses cleanly
- Start with Windows: test failure path (run without admin rights to trigger registry error) — checkbox reverts, error box appears
- Click Apply after checkbox changes: no double-apply side effects
