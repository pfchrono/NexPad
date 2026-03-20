/*-------------------------------------------------------------------------------
  NexPad is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
---------------------------------------------------------------------------------*/

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <uxtheme.h>

#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "winmm")
#pragma comment(linker, "/ENTRY:wWinMainCRTStartup")
#pragma comment(linker, "/SUBSYSTEM:WINDOWS")

#include "NexPad.h"
#include "resource.h"

bool ChangeVolume(double nVolume, bool bScalar);
BOOL isRunningAsAdministrator();

namespace
{
  enum ControlId
  {
    IDC_MAIN_TAB = 1001,
    IDC_STATUS_TEXT,
    IDC_CONTROLLER_TEXT,
    IDC_CONTROLLER_TYPE_TEXT,
    IDC_BATTERY_TEXT,
    IDC_SPEED_TEXT,
    IDC_SCROLL_TEXT,
    IDC_CONFIG_TEXT,
    IDC_TOGGLE_BUTTON,
    IDC_STARTUP_INFO,
    IDC_OUTPUT_INFO,
    IDC_SPEED_COMBO,
    IDC_SCROLL_EDIT,
    IDC_TOUCHPAD_SPEED_EDIT,
    IDC_TOUCHPAD_DEAD_ZONE_EDIT,
    IDC_TOUCHPAD_CHECK,
    IDC_SWAP_CHECK,
    IDC_PRESET_LIST,
    IDC_PRESET_NAME_EDIT,
    IDC_PRESET_REFRESH_BUTTON,
    IDC_PRESET_SAVE_BUTTON,
    IDC_PRESET_DELETE_BUTTON,
    IDC_MAPPING_KEY_COMBO,
    IDC_MAPPING_VALUE_COMBO,
    IDC_MAPPING_DESC_TEXT,
    IDC_APPLY_MAPPING_BUTTON,
    IDC_MAPPINGS_HELP,
    IDC_APPLY_BUTTON,
    IDC_SAVE_BUTTON,
    IDC_RELOAD_BUTTON,
    IDC_IMPORT_BUTTON,
    IDC_EXPORT_BUTTON,
    IDC_SETTINGS_NOTE
  };

  const UINT WMAPP_TRAYICON = WM_APP + 1;
  const UINT ID_TRAY_ICON = 1;
  const UINT ID_TRAY_RESTORE = 40001;
  const UINT ID_TRAY_EXIT = 40002;

  struct GuiState
  {
    enum class ThemeMode
    {
      Light,
      Dark
    };

    CXBOXController controller;
    NexPad nexPad;
    HWND tab = NULL;
    HWND statusPage = NULL;
    HWND settingsPage = NULL;
    HWND mappingsPage = NULL;
    HWND statusText = NULL;
    HWND controllerText = NULL;
    HWND controllerTypeText = NULL;
    HWND batteryText = NULL;
    HWND speedText = NULL;
    HWND scrollText = NULL;
    HWND configText = NULL;
    HWND toggleButton = NULL;
    HWND startupInfo = NULL;
    HWND outputInfo = NULL;
    HWND speedCombo = NULL;
    HWND scrollEdit = NULL;
    HWND touchpadSpeedEdit = NULL;
    HWND touchpadDeadZoneEdit = NULL;
    HWND touchpadCheck = NULL;
    HWND swapCheck = NULL;
    HWND presetList = NULL;
    HWND presetNameEdit = NULL;
    HWND presetRefreshButton = NULL;
    HWND presetSaveButton = NULL;
    HWND presetDeleteButton = NULL;
    HWND mappingKeyCombo = NULL;
    HWND mappingValueCombo = NULL;
    HWND mappingDescription = NULL;
    HWND applyMappingButton = NULL;
    HWND mappingsHelp = NULL;
    HWND applyButton = NULL;
    HWND saveButton = NULL;
    HWND reloadButton = NULL;
    HWND importButton = NULL;
    HWND exportButton = NULL;
    HWND settingsNote = NULL;
    HWND speedLabel = NULL;
    HWND scrollLabel = NULL;
    HWND touchpadSpeedLabel = NULL;
    HWND touchpadDeadZoneLabel = NULL;
    HWND presetListLabel = NULL;
    HWND presetNameLabel = NULL;
    HFONT font = NULL;
    NOTIFYICONDATAA trayIconData = {};
    bool trayAdded = false;
    ThemeMode themeMode = ThemeMode::Dark;
    COLORREF backgroundColor = RGB(0, 0, 0);
    COLORREF panelColor = RGB(0, 0, 0);
    COLORREF textColor = RGB(255, 255, 255);
    COLORREF mutedTextColor = RGB(170, 170, 170);
    COLORREF editBackgroundColor = RGB(0, 0, 0);
    COLORREF buttonColor = RGB(0, 0, 0);
    COLORREF buttonTextColor = RGB(255, 255, 255);
    HBRUSH backgroundBrush = NULL;
    HBRUSH panelBrush = NULL;
    HBRUSH editBrush = NULL;
    HBRUSH buttonBrush = NULL;
    HWND hotButton = NULL;
    HWND pressedButton = NULL;
    HWND hotCombo = NULL;
    int hotTabIndex = -1;
    std::string cachedStatusText;
    std::string cachedControllerText;
    std::string cachedControllerTypeText;
    std::string cachedBatteryText;
    std::string cachedSpeedText;
    std::string cachedScrollText;
    std::string cachedConfigText;
    std::string cachedToggleButtonText;
    std::string cachedTrayTooltip;

    GuiState()
      : controller(1),
        nexPad(&controller)
    {
    }
  };

  const bool kUseNativeComboPrototype = true;

  void applyControlTheme(HWND control);
  std::string getComboItemText(HWND comboBox, UINT itemId);

  void applyEditPadding(HWND control, int horizontalPadding = 8, int verticalPadding = 6)
  {
    if (control == NULL)
    {
      return;
    }

    const LONG style = GetWindowLongA(control, GWL_STYLE);
    if ((style & ES_MULTILINE) != 0)
    {
      RECT rect = {};
      GetClientRect(control, &rect);
      rect.left += horizontalPadding;
      rect.right -= horizontalPadding;
      rect.top += verticalPadding;
      rect.bottom -= verticalPadding;

      if (rect.right <= rect.left)
      {
        rect.right = rect.left + 1;
      }

      if (rect.bottom <= rect.top)
      {
        rect.bottom = rect.top + 1;
      }

      SendMessageA(control, EM_SETRECTNP, 0, reinterpret_cast<LPARAM>(&rect));
      InvalidateRect(control, NULL, TRUE);
      return;
    }

    SendMessageA(control, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(horizontalPadding, horizontalPadding));
  }

  bool isEditControl(HWND control)
  {
    if (control == NULL)
    {
      return false;
    }

    char className[16] = {};
    const int length = GetClassNameA(control, className, static_cast<int>(sizeof(className)));
    return length > 0 && _stricmp(className, "Edit") == 0;
  }

  std::string getPresetDirectory()
  {
    return "presets";
  }

  void ensurePresetDirectory()
  {
    CreateDirectoryA(getPresetDirectory().c_str(), NULL);
  }

  std::vector<std::string> enumeratePresetFiles()
  {
    ensurePresetDirectory();

    std::vector<std::string> presets;
    WIN32_FIND_DATAA findData;
    HANDLE findHandle = FindFirstFileA((getPresetDirectory() + "\\*.*").c_str(), &findData);
    if (findHandle == INVALID_HANDLE_VALUE)
    {
      return presets;
    }

    do
    {
      if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
      {
        continue;
      }

      std::string name = findData.cFileName;
      const size_t dot = name.find_last_of('.');
      if (dot == std::string::npos)
      {
        continue;
      }

      const std::string extension = name.substr(dot + 1);
      if (_stricmp(extension.c_str(), "ini") == 0 || _stricmp(extension.c_str(), "txt") == 0)
      {
        presets.push_back(name);
      }
    } while (FindNextFileA(findHandle, &findData));

    FindClose(findHandle);
    std::sort(presets.begin(), presets.end());
    return presets;
  }

  GuiState* getGuiState(HWND window)
  {
    return reinterpret_cast<GuiState*>(GetWindowLongPtr(window, GWLP_USERDATA));
  }

  void updateTrayTooltip(HWND window);

  void beginMouseTracking(HWND window)
  {
    TRACKMOUSEEVENT track = {};
    track.cbSize = sizeof(track);
    track.dwFlags = TME_LEAVE;
    track.hwndTrack = window;
    TrackMouseEvent(&track);
  }

  void invalidateIfNotNull(HWND window)
  {
    if (window != NULL)
    {
      InvalidateRect(window, NULL, TRUE);
    }
  }

  HWND getMainWindowFromPage(HWND page)
  {
    HWND tab = GetParent(page);
    return tab != NULL ? GetParent(tab) : NULL;
  }

  LRESULT CALLBACK pageSubclassProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
  {
    HWND mainWindow = getMainWindowFromPage(window);
    GuiState* state = mainWindow != NULL ? getGuiState(mainWindow) : NULL;

    switch (message)
    {
    case WM_MEASUREITEM:
    case WM_DRAWITEM:
    case WM_COMPAREITEM:
    case WM_DELETEITEM:
    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
    case WM_COMMAND:
    case WM_NOTIFY:
      if (mainWindow != NULL)
      {
        return SendMessage(mainWindow, message, wParam, lParam);
      }
      break;

    case WM_ERASEBKGND:
      if (state && state->panelBrush != NULL)
      {
        RECT clientRect;
        GetClientRect(window, &clientRect);
        FillRect(reinterpret_cast<HDC>(wParam), &clientRect, state->panelBrush);
        return 1;
      }
      break;

    case WM_PAINT:
      if (state && state->panelBrush != NULL)
      {
        PAINTSTRUCT paint = {};
        HDC dc = BeginPaint(window, &paint);
        FillRect(dc, &paint.rcPaint, state->panelBrush);
        EndPaint(window, &paint);
        return 0;
      }
      break;

    case WM_PRINTCLIENT:
      if (state && state->panelBrush != NULL)
      {
        RECT clientRect;
        GetClientRect(window, &clientRect);
        FillRect(reinterpret_cast<HDC>(wParam), &clientRect, state->panelBrush);
        return 0;
      }
      break;

    case WM_CTLCOLORSTATIC:
      if (state)
      {
        HDC dc = reinterpret_cast<HDC>(wParam);

        if (isEditControl(reinterpret_cast<HWND>(lParam)))
        {
          SetBkMode(dc, OPAQUE);
          SetTextColor(dc, state->textColor);
          SetBkColor(dc, state->editBackgroundColor);
          return reinterpret_cast<LRESULT>(state->editBrush);
        }

        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, state->textColor);
        SetBkColor(dc, state->panelColor);
        return reinterpret_cast<LRESULT>(state->panelBrush);
      }
      break;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
      if (state)
      {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, state->textColor);
        SetBkColor(dc, state->editBackgroundColor);
        return reinterpret_cast<LRESULT>(state->editBrush);
      }
      break;

    case WM_CTLCOLORBTN:
      if (state)
      {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, state->buttonTextColor);
        SetBkColor(dc, state->buttonColor);
        return reinterpret_cast<LRESULT>(state->buttonBrush);
      }
      break;
    }

    return DefSubclassProc(window, message, wParam, lParam);
  }

  LRESULT CALLBACK buttonSubclassProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR refData)
  {
    GuiState* state = reinterpret_cast<GuiState*>(refData);
    if (!state)
    {
      return DefSubclassProc(window, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_ERASEBKGND:
      if (state && state->editBrush != NULL)
      {
        RECT clientRect;
        GetClientRect(window, &clientRect);
        FillRect(reinterpret_cast<HDC>(wParam), &clientRect, state->editBrush);
        return 1;
      }
      break;

    case WM_MOUSEMOVE:
      beginMouseTracking(window);
      if (state->hotButton != window)
      {
        HWND previous = state->hotButton;
        state->hotButton = window;
        invalidateIfNotNull(previous);
        InvalidateRect(window, NULL, TRUE);
      }
      break;

    case WM_MOUSELEAVE:
      if (state->hotButton == window)
      {
        state->hotButton = NULL;
        InvalidateRect(window, NULL, TRUE);
      }
      break;

    case WM_LBUTTONDOWN:
      state->pressedButton = window;
      InvalidateRect(window, NULL, TRUE);
      break;

    case WM_LBUTTONUP:
      if (state->pressedButton == window)
      {
        state->pressedButton = NULL;
        InvalidateRect(window, NULL, TRUE);
      }
      break;
    }

    return DefSubclassProc(window, message, wParam, lParam);
  }

  void drawComboChrome(HWND window, const GuiState* state, HDC targetDc = NULL)
  {
    if (!window || !state)
    {
      return;
    }

    RECT rect;
    GetWindowRect(window, &rect);
    OffsetRect(&rect, -rect.left, -rect.top);

    const int buttonWidth = GetSystemMetrics(SM_CXVSCROLL);
    RECT buttonRect = rect;
    buttonRect.left = max(rect.left, rect.right - buttonWidth);

    const bool enabled = IsWindowEnabled(window) != FALSE;
    const bool hot = state->hotCombo == window;
    const bool dropped = SendMessage(window, CB_GETDROPPEDSTATE, 0, 0) != 0;
    const COLORREF borderColor = !enabled ? state->mutedTextColor : (hot || dropped ? state->textColor : state->mutedTextColor);
    const COLORREF arrowFill = state->editBackgroundColor;
    const COLORREF arrowColor = enabled ? state->buttonTextColor : state->mutedTextColor;

    HDC dc = targetDc != NULL ? targetDc : GetWindowDC(window);
    if (!dc)
    {
      return;
    }

    RECT contentRect = rect;
    contentRect.right = buttonRect.left;
    HBRUSH contentBrush = CreateSolidBrush(state->editBackgroundColor);
    FillRect(dc, &contentRect, contentBrush);
    DeleteObject(contentBrush);

    HBRUSH buttonBrush = CreateSolidBrush(arrowFill);
    FillRect(dc, &buttonRect, buttonBrush);
    DeleteObject(buttonBrush);

    const std::string text = getComboItemText(window, static_cast<UINT>(-1));
    RECT textRect = contentRect;
    textRect.left += 6;
    textRect.right -= 4;
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, enabled ? state->textColor : state->mutedTextColor);
    HGDIOBJ oldFont = NULL;
    if (state->font != NULL)
    {
      oldFont = SelectObject(dc, state->font);
    }
    DrawTextA(dc, text.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
    if (oldFont != NULL)
    {
      SelectObject(dc, oldFont);
    }

    HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
    MoveToEx(dc, buttonRect.left, buttonRect.top, NULL);
    LineTo(dc, buttonRect.left, buttonRect.bottom);

    const int centerX = (buttonRect.left + buttonRect.right) / 2;
    const int centerY = (buttonRect.top + buttonRect.bottom) / 2;
    POINT triangle[3] =
    {
      { centerX - 4, centerY - 2 },
      { centerX + 4, centerY - 2 },
      { centerX, centerY + 3 }
    };
    HBRUSH triangleBrush = CreateSolidBrush(arrowColor);
    SelectObject(dc, triangleBrush);
    Polygon(dc, triangle, 3);
    DeleteObject(triangleBrush);

    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(pen);
    if (targetDc == NULL)
    {
      ReleaseDC(window, dc);
    }
  }

  void refreshComboDisplay(HWND combo)
  {
    if (combo == NULL)
    {
      return;
    }

    RedrawWindow(combo, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME);

    COMBOBOXINFO info = {};
    info.cbSize = sizeof(info);
    if (!GetComboBoxInfo(combo, &info))
    {
      return;
    }

    if (info.hwndItem != NULL)
    {
      RedrawWindow(info.hwndItem, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME);
    }
  }

  std::string getComboItemText(HWND comboBox, UINT itemId)
  {
    if (comboBox == NULL)
    {
      return std::string();
    }

    if (itemId == static_cast<UINT>(-1))
    {
      const LRESULT currentSelection = SendMessage(comboBox, CB_GETCURSEL, 0, 0);
      if (currentSelection != CB_ERR)
      {
        itemId = static_cast<UINT>(currentSelection);
      }
      else
      {
        char text[256] = {};
        GetWindowTextA(comboBox, text, static_cast<int>(sizeof(text)));
        return text;
      }
    }

    const LRESULT textLength = SendMessage(comboBox, CB_GETLBTEXTLEN, itemId, 0);
    if (textLength == CB_ERR)
    {
      return std::string();
    }

    std::string text(static_cast<size_t>(textLength) + 1, '\0');
    SendMessageA(comboBox, CB_GETLBTEXT, itemId, reinterpret_cast<LPARAM>(&text[0]));
    text.resize(static_cast<size_t>(textLength));
    return text;
  }

  HWND resolveComboHandle(const DRAWITEMSTRUCT* drawItem, const GuiState* state)
  {
    if (!drawItem || !state)
    {
      return NULL;
    }

    if (drawItem->CtlType == ODT_COMBOBOX)
    {
      return drawItem->hwndItem;
    }

    if (drawItem->CtlType != ODT_LISTBOX)
    {
      return NULL;
    }

    const HWND combos[] =
    {
      state->speedCombo,
      state->presetList,
      state->mappingKeyCombo,
      state->mappingValueCombo
    };

    for (HWND combo : combos)
    {
      if (combo == NULL)
      {
        continue;
      }

      COMBOBOXINFO info = {};
      info.cbSize = sizeof(info);
      if (GetComboBoxInfo(combo, &info) && info.hwndList == drawItem->hwndItem)
      {
        return combo;
      }
    }

    return NULL;
  }

  void drawThemedComboItem(const DRAWITEMSTRUCT* drawItem, const GuiState* state)
  {
    if (!drawItem || !state)
    {
      return;
    }

    HWND comboHandle = resolveComboHandle(drawItem, state);
    if (comboHandle == NULL)
    {
      return;
    }

    HDC dc = drawItem->hDC;
    RECT rect = drawItem->rcItem;
    const bool enabled = IsWindowEnabled(comboHandle) != FALSE;
    const bool selected = (drawItem->itemState & ODS_SELECTED) != 0;
    const bool focused = (drawItem->itemState & ODS_FOCUS) != 0;

    COLORREF fillColor = state->editBackgroundColor;
    COLORREF textColor = enabled ? state->textColor : state->mutedTextColor;

    HBRUSH brush = CreateSolidBrush(fillColor);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);

    const std::string text = getComboItemText(comboHandle, drawItem->itemID);

    RECT textRect = rect;
    textRect.left += 6;
    if (state->font != NULL)
    {
      SelectObject(dc, state->font);
    }
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, textColor);
    DrawTextA(dc, text.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

    if (selected)
    {
      HPEN selectionPen = CreatePen(PS_SOLID, 1, state->textColor);
      HGDIOBJ oldPen = SelectObject(dc, selectionPen);
      HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
      Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
      SelectObject(dc, oldBrush);
      SelectObject(dc, oldPen);
      DeleteObject(selectionPen);
    }

    if (focused)
    {
      RECT focusRect = rect;
      InflateRect(&focusRect, -2, -2);
      DrawFocusRect(dc, &focusRect);
    }
  }

  LRESULT CALLBACK comboSubclassProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR refData)
  {
    GuiState* state = reinterpret_cast<GuiState*>(refData);
    if (!state)
    {
      return DefSubclassProc(window, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_ERASEBKGND:
      if (state && state->editBrush != NULL)
      {
        RECT clientRect;
        GetClientRect(window, &clientRect);
        FillRect(reinterpret_cast<HDC>(wParam), &clientRect, state->editBrush);
        return 1;
      }
      break;

    case WM_MOUSEMOVE:
      beginMouseTracking(window);
      if (state->hotCombo != window)
      {
        HWND previous = state->hotCombo;
        state->hotCombo = window;
        invalidateIfNotNull(previous);
        InvalidateRect(window, NULL, TRUE);
      }
      break;

    case WM_MOUSELEAVE:
      if (state->hotCombo == window)
      {
        state->hotCombo = NULL;
        InvalidateRect(window, NULL, TRUE);
      }
      break;

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
      {
        const LRESULT result = DefSubclassProc(window, message, wParam, lParam);
        InvalidateRect(window, NULL, TRUE);
        return result;
      }

    case WM_PAINT:
    case WM_NCPAINT:
      {
        const LRESULT result = DefSubclassProc(window, message, wParam, lParam);
        drawComboChrome(window, state);
        return result;
      }

    case WM_PRINTCLIENT:
      {
        const LRESULT result = DefSubclassProc(window, message, wParam, lParam);
        drawComboChrome(window, state, reinterpret_cast<HDC>(wParam));
        return result;
      }
    }

    return DefSubclassProc(window, message, wParam, lParam);
  }

  LRESULT CALLBACK comboListSubclassProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR refData)
  {
    GuiState* state = reinterpret_cast<GuiState*>(refData);
    if (!state)
    {
      return DefSubclassProc(window, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_ERASEBKGND:
      if (state->editBrush != NULL)
      {
        RECT clientRect;
        GetClientRect(window, &clientRect);
        FillRect(reinterpret_cast<HDC>(wParam), &clientRect, state->editBrush);
        return 1;
      }
      break;

    case WM_PRINTCLIENT:
      if (state->editBrush != NULL)
      {
        RECT clientRect;
        GetClientRect(window, &clientRect);
        FillRect(reinterpret_cast<HDC>(wParam), &clientRect, state->editBrush);
        return 0;
      }
      break;

    case WM_PAINT:
      {
        const LRESULT result = DefSubclassProc(window, message, wParam, lParam);
        HDC dc = GetWindowDC(window);
        if (dc != NULL)
        {
          RECT rect;
          GetWindowRect(window, &rect);
          OffsetRect(&rect, -rect.left, -rect.top);
          HPEN pen = CreatePen(PS_SOLID, 1, state->mutedTextColor);
          HGDIOBJ oldPen = SelectObject(dc, pen);
          HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
          Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
          SelectObject(dc, oldBrush);
          SelectObject(dc, oldPen);
          DeleteObject(pen);
          ReleaseDC(window, dc);
        }
        return result;
      }
    }

    return DefSubclassProc(window, message, wParam, lParam);
  }

  LRESULT CALLBACK tabSubclassProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR refData)
  {
    GuiState* state = reinterpret_cast<GuiState*>(refData);
    if (!state)
    {
      return DefSubclassProc(window, message, wParam, lParam);
    }

    switch (message)
    {
    case WM_ERASEBKGND:
      if (state && state->panelBrush != NULL)
      {
        RECT clientRect;
        GetClientRect(window, &clientRect);
        FillRect(reinterpret_cast<HDC>(wParam), &clientRect, state->panelBrush);
        return 1;
      }
      break;

    case WM_MOUSEMOVE:
      {
        beginMouseTracking(window);
        TCHITTESTINFO hitTest = {};
        hitTest.pt.x = GET_X_LPARAM(lParam);
        hitTest.pt.y = GET_Y_LPARAM(lParam);
        const int hotIndex = static_cast<int>(SendMessage(window, TCM_HITTEST, 0, reinterpret_cast<LPARAM>(&hitTest)));
        if (state->hotTabIndex != hotIndex)
        {
          state->hotTabIndex = hotIndex;
          InvalidateRect(window, NULL, TRUE);
        }
      }
      break;

    case WM_MOUSELEAVE:
      if (state->hotTabIndex != -1)
      {
        state->hotTabIndex = -1;
        InvalidateRect(window, NULL, TRUE);
      }
      break;
    }

    return DefSubclassProc(window, message, wParam, lParam);
  }

  void destroyThemeResources(GuiState* state)
  {
    if (!state)
    {
      return;
    }

    if (state->backgroundBrush != NULL)
    {
      DeleteObject(state->backgroundBrush);
      state->backgroundBrush = NULL;
    }
    if (state->panelBrush != NULL)
    {
      DeleteObject(state->panelBrush);
      state->panelBrush = NULL;
    }
    if (state->editBrush != NULL)
    {
      DeleteObject(state->editBrush);
      state->editBrush = NULL;
    }
    if (state->buttonBrush != NULL)
    {
      DeleteObject(state->buttonBrush);
      state->buttonBrush = NULL;
    }
  }

  void applyThemePalette(GuiState* state)
  {
    if (!state)
    {
      return;
    }

    destroyThemeResources(state);

    state->themeMode = GuiState::ThemeMode::Dark;
    state->backgroundColor = RGB(0, 0, 0);
    state->panelColor = RGB(0, 0, 0);
    state->textColor = RGB(255, 255, 255);
    state->mutedTextColor = RGB(170, 170, 170);
    state->editBackgroundColor = RGB(0, 0, 0);
    state->buttonColor = RGB(0, 0, 0);
    state->buttonTextColor = RGB(255, 255, 255);

    state->backgroundBrush = CreateSolidBrush(state->backgroundColor);
    state->panelBrush = CreateSolidBrush(state->panelColor);
    state->editBrush = CreateSolidBrush(state->editBackgroundColor);
    state->buttonBrush = CreateSolidBrush(state->buttonColor);
  }

  void applyNativeTabPalette(HWND tab, const GuiState* state)
  {
    if (tab == NULL || state == NULL)
    {
      return;
    }

    applyControlTheme(tab);
    InvalidateRect(tab, NULL, TRUE);
  }

  void refreshTheme(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    applyThemePalette(state);
    applyNativeTabPalette(state->tab, state);
    InvalidateRect(window, NULL, TRUE);
    if (state->tab != NULL)
    {
      InvalidateRect(state->tab, NULL, TRUE);
    }
    if (state->statusPage != NULL)
    {
      InvalidateRect(state->statusPage, NULL, TRUE);
    }
    if (state->settingsPage != NULL)
    {
      InvalidateRect(state->settingsPage, NULL, TRUE);
    }
    if (state->mappingsPage != NULL)
    {
      InvalidateRect(state->mappingsPage, NULL, TRUE);
    }
    invalidateIfNotNull(state->toggleButton);
    invalidateIfNotNull(state->applyButton);
    invalidateIfNotNull(state->saveButton);
    invalidateIfNotNull(state->reloadButton);
    invalidateIfNotNull(state->presetRefreshButton);
    invalidateIfNotNull(state->presetSaveButton);
    invalidateIfNotNull(state->presetDeleteButton);
    invalidateIfNotNull(state->importButton);
    invalidateIfNotNull(state->exportButton);
    invalidateIfNotNull(state->applyMappingButton);
    invalidateIfNotNull(state->speedCombo);
    invalidateIfNotNull(state->presetList);
    invalidateIfNotNull(state->mappingKeyCombo);
    invalidateIfNotNull(state->mappingValueCombo);
    UpdateWindow(window);
  }

  void applyControlTheme(HWND control)
  {
    if (control != NULL)
    {
      SetWindowTheme(control, L"", L"");
    }
  }

  void applyComboChildTheme(HWND combo, HFONT font, bool readOnlyInput = false)
  {
    if (combo == NULL)
    {
      return;
    }

    COMBOBOXINFO info = {};
    info.cbSize = sizeof(info);
    if (!GetComboBoxInfo(combo, &info))
    {
      return;
    }

    applyControlTheme(info.hwndList);
    applyControlTheme(info.hwndItem);

    if (readOnlyInput && info.hwndItem != NULL)
    {
      SendMessage(info.hwndItem, EM_SETREADONLY, TRUE, 0);
    }

    if (!kUseNativeComboPrototype && info.hwndList != NULL)
    {
      HWND ownerWindow = GetParent(combo);
      if (ownerWindow != NULL)
      {
        ownerWindow = getMainWindowFromPage(ownerWindow);
      }
      GuiState* state = ownerWindow != NULL ? getGuiState(ownerWindow) : NULL;
      if (state != NULL)
      {
        SetWindowSubclass(info.hwndList, comboListSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
      }
    }

    if (font != NULL)
    {
      if (info.hwndList != NULL)
      {
        SendMessage(info.hwndList, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
      }
      if (info.hwndItem != NULL)
      {
        SendMessage(info.hwndItem, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
      }
    }
  }

  void drawThemedButton(const DRAWITEMSTRUCT* drawItem, const GuiState* state)
  {
    if (!drawItem || !state)
    {
      return;
    }

    HDC dc = drawItem->hDC;
    RECT rect = drawItem->rcItem;
    COLORREF fillColor = state->buttonColor;
    COLORREF textColor = state->buttonTextColor;

    COLORREF borderColor = state->mutedTextColor;
    if (state->pressedButton == drawItem->hwndItem || (drawItem->itemState & ODS_SELECTED) != 0)
    {
      borderColor = state->textColor;
    }
    else if (state->hotButton == drawItem->hwndItem || (drawItem->itemState & ODS_HOTLIGHT) != 0)
    {
      borderColor = RGB(220, 220, 220);
    }

    if (drawItem->hwndItem == state->swapCheck)
    {
      const bool checked = SendMessage(state->swapCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
      HBRUSH backgroundBrush = CreateSolidBrush(fillColor);
      FillRect(dc, &rect, backgroundBrush);
      DeleteObject(backgroundBrush);

      RECT boxRect = rect;
      boxRect.left += 4;
      boxRect.top += (rect.bottom - rect.top - 16) / 2;
      boxRect.right = boxRect.left + 16;
      boxRect.bottom = boxRect.top + 16;

      HPEN boxPen = CreatePen(PS_SOLID, 1, checked ? state->textColor : borderColor);
      HGDIOBJ oldPen = SelectObject(dc, boxPen);
      HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
      Rectangle(dc, boxRect.left, boxRect.top, boxRect.right, boxRect.bottom);
      SelectObject(dc, oldBrush);

      if (checked)
      {
        MoveToEx(dc, boxRect.left + 3, boxRect.top + 8, NULL);
        LineTo(dc, boxRect.left + 7, boxRect.bottom - 4);
        LineTo(dc, boxRect.right - 3, boxRect.top + 4);
      }

      char text[256] = {};
      GetWindowTextA(drawItem->hwndItem, text, static_cast<int>(sizeof(text)));
      RECT textRect = rect;
      textRect.left = boxRect.right + 8;
      SetBkMode(dc, TRANSPARENT);
      SetTextColor(dc, textColor);
      DrawTextA(dc, text, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

      if ((drawItem->itemState & ODS_FOCUS) != 0)
      {
        RECT focusRect = textRect;
        InflateRect(&focusRect, -2, -2);
        DrawFocusRect(dc, &focusRect);
      }

      SelectObject(dc, oldPen);
      DeleteObject(boxPen);
      return;
    }

    HBRUSH brush = CreateSolidBrush(fillColor);
    FillRect(dc, &rect, brush);
    DeleteObject(brush);

    HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(HOLLOW_BRUSH));
    Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(pen);

    char text[256] = {};
    GetWindowTextA(drawItem->hwndItem, text, static_cast<int>(sizeof(text)));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, textColor);
    DrawTextA(dc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    if ((drawItem->itemState & ODS_FOCUS) != 0)
    {
      RECT focusRect = rect;
      InflateRect(&focusRect, -3, -3);
      DrawFocusRect(dc, &focusRect);
    }
  }

  void drawThemedTabItem(const DRAWITEMSTRUCT* drawItem, const GuiState* state)
  {
    if (!drawItem || !state)
    {
      return;
    }

    RECT rect = drawItem->rcItem;
    const int tabIndex = static_cast<int>(drawItem->itemID);
    const bool selected = TabCtrl_GetCurSel(drawItem->hwndItem) == tabIndex;
    const bool hot = state->hotTabIndex == tabIndex;

    TCITEMA item = {};
    char text[128] = {};
    item.mask = TCIF_TEXT;
    item.pszText = text;
    item.cchTextMax = static_cast<int>(sizeof(text));
    SendMessageA(drawItem->hwndItem, TCM_GETITEMA, tabIndex, reinterpret_cast<LPARAM>(&item));

    COLORREF borderColor = state->mutedTextColor;
    if (selected)
    {
      borderColor = state->textColor;
    }
    else if (hot)
    {
      borderColor = RGB(220, 220, 220);
    }

    HBRUSH backgroundBrush = CreateSolidBrush(state->panelColor);
    FillRect(drawItem->hDC, &drawItem->rcItem, backgroundBrush);
    DeleteObject(backgroundBrush);

    HBRUSH brush = CreateSolidBrush(state->panelColor);
    HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
    HGDIOBJ oldPen = SelectObject(drawItem->hDC, pen);
    HGDIOBJ oldBrush = SelectObject(drawItem->hDC, brush);
    RoundRect(drawItem->hDC, rect.left, rect.top + (selected ? 0 : 3), rect.right, rect.bottom + 2, 6, 6);
    SelectObject(drawItem->hDC, oldBrush);
    SelectObject(drawItem->hDC, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    if (selected)
    {
      RECT coverRect = rect;
      coverRect.top = rect.bottom - 2;
      coverRect.bottom = rect.bottom + 1;
      HBRUSH coverBrush = CreateSolidBrush(state->panelColor);
      FillRect(drawItem->hDC, &coverRect, coverBrush);
      DeleteObject(coverBrush);
    }

    HGDIOBJ oldFont = NULL;
    if (state->font != NULL)
    {
      oldFont = SelectObject(drawItem->hDC, state->font);
    }

    SetBkMode(drawItem->hDC, TRANSPARENT);
    SetTextColor(drawItem->hDC, state->textColor);
    rect.left += 4;
    rect.right -= 4;
    rect.top += 1;
    if (selected)
    {
      rect.bottom -= 2;
    }
    DrawTextA(drawItem->hDC, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

    if (oldFont != NULL)
    {
      SelectObject(drawItem->hDC, oldFont);
    }

    if ((drawItem->itemState & ODS_FOCUS) != 0)
    {
      RECT focusRect = drawItem->rcItem;
      InflateRect(&focusRect, -3, -3);
      DrawFocusRect(drawItem->hDC, &focusRect);
    }
  }

  LRESULT handleTabCustomDraw(const NMCUSTOMDRAW* customDraw, const GuiState* state)
  {
    if (!customDraw || !state)
    {
      return CDRF_DODEFAULT;
    }

    switch (customDraw->dwDrawStage)
    {
    case CDDS_PREPAINT:
      {
        RECT clientRect;
        GetClientRect(customDraw->hdr.hwndFrom, &clientRect);
        HBRUSH backgroundBrush = CreateSolidBrush(state->panelColor);
        FillRect(customDraw->hdc, &clientRect, backgroundBrush);
        DeleteObject(backgroundBrush);

        HPEN stripPen = CreatePen(PS_SOLID, 1, state->mutedTextColor);
        HGDIOBJ oldPen = SelectObject(customDraw->hdc, stripPen);
        MoveToEx(customDraw->hdc, clientRect.left, clientRect.bottom - 1, NULL);
        LineTo(customDraw->hdc, clientRect.right, clientRect.bottom - 1);
        SelectObject(customDraw->hdc, oldPen);
        DeleteObject(stripPen);
      }
      return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT:
      {
        const int tabIndex = static_cast<int>(customDraw->dwItemSpec);
        RECT rect;
        TabCtrl_GetItemRect(customDraw->hdr.hwndFrom, tabIndex, &rect);

        TCITEMA item = {};
        char text[128] = {};
        item.mask = TCIF_TEXT;
        item.pszText = text;
        item.cchTextMax = static_cast<int>(sizeof(text));
        SendMessageA(customDraw->hdr.hwndFrom, TCM_GETITEMA, tabIndex, reinterpret_cast<LPARAM>(&item));

        const bool selected = (customDraw->uItemState & CDIS_SELECTED) != 0;
        const bool hot = state->hotTabIndex == tabIndex;
        COLORREF fillColor = state->panelColor;
        COLORREF borderColor = state->mutedTextColor;
        if (selected)
        {
          borderColor = state->textColor;
        }
        else if (hot)
        {
          borderColor = RGB(220, 220, 220);
        }
        HBRUSH brush = CreateSolidBrush(fillColor);
        HPEN pen = CreatePen(PS_SOLID, 1, borderColor);
        HGDIOBJ oldPen = SelectObject(customDraw->hdc, pen);
        HGDIOBJ oldBrush = SelectObject(customDraw->hdc, brush);
        RoundRect(customDraw->hdc, rect.left, rect.top + (selected ? 0 : 3), rect.right, rect.bottom + 2, 8, 8);
        SelectObject(customDraw->hdc, oldBrush);
        SelectObject(customDraw->hdc, oldPen);
        DeleteObject(brush);
        DeleteObject(pen);

        if (selected)
        {
          RECT coverRect = rect;
          coverRect.top = rect.bottom - 2;
          coverRect.bottom = rect.bottom + 1;
          HBRUSH coverBrush = CreateSolidBrush(state->panelColor);
          FillRect(customDraw->hdc, &coverRect, coverBrush);
          DeleteObject(coverBrush);
        }

        HGDIOBJ oldFont = NULL;
        if (state->font != NULL)
        {
          oldFont = SelectObject(customDraw->hdc, state->font);
        }
        SetBkMode(customDraw->hdc, TRANSPARENT);
        SetTextColor(customDraw->hdc, state->textColor);
        rect.left += 8;
        rect.right -= 8;
        if (selected)
        {
          rect.bottom -= 2;
        }
        DrawTextA(customDraw->hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (selected)
        {
          RECT accentRect = rect;
          accentRect.top = customDraw->rc.top;
          accentRect.bottom = accentRect.top + 3;
          HBRUSH accentBrush = CreateSolidBrush(state->textColor);
          FillRect(customDraw->hdc, &accentRect, accentBrush);
          DeleteObject(accentBrush);
        }
        if (oldFont != NULL)
        {
          SelectObject(customDraw->hdc, oldFont);
        }
        return CDRF_SKIPDEFAULT;
      }
    }

    return CDRF_DODEFAULT;
  }

  void setTextIfChanged(HWND control, std::string& cache, const std::string& value)
  {
    if (control == NULL || cache == value)
    {
      return;
    }

    cache = value;
    SetWindowTextA(control, value.c_str());
  }

  void setControlFont(HFONT font, std::initializer_list<HWND> controls)
  {
    for (HWND control : controls)
    {
      if (control != NULL)
      {
        SendMessage(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
      }
    }
  }

  std::string buildStartupInfo()
  {
    std::ostringstream stream;
    stream << "Welcome to NexPad - a lightweight controller-to-keyboard and mouse input tool.\r\n\r\n";
    stream << "Supported controllers:\r\n";
    stream << "- Xbox 360 / One / Series through XInput on Windows 10/11\r\n";
    stream << "- PlayStation 5 DualSense through XInput bridge tools or native HID fallback\r\n\r\n";
    stream << "Tip - Press left and right bumpers simultaneously to toggle speeds.\r\n";
    stream << "Tip - Use the Settings tab to change speed presets, scroll speed, and thumbstick swap live.\r\n";
    stream << "Tip - Minimize the window to keep NexPad running in the notification area.\r\n";

    if (!isRunningAsAdministrator())
    {
      stream << "Tip - Not running as administrator; the On-Screen Keyboard may not be clickable.\r\n";
    }

    stream << "\r\nConfig file: config.ini";
    return stream.str();
  }

  std::string buildPresetHelpText()
  {
    return "Manage named preset profiles stored in the presets folder. Select one to load, enter a name to save, or delete the selected preset.";
  }

  std::vector<std::pair<DWORD, std::string>> buildGamepadValueOptions()
  {
    return
    {
      { 0x0000, "0x0000 - None" },
      { 0x0001, "0x0001 - DPad Up" },
      { 0x0002, "0x0002 - DPad Down" },
      { 0x0004, "0x0004 - DPad Left" },
      { 0x0008, "0x0008 - DPad Right" },
      { 0x0010, "0x0010 - Start / Options" },
      { 0x0020, "0x0020 - Back / Share" },
      { 0x0040, "0x0040 - Left Thumb" },
      { 0x0080, "0x0080 - Right Thumb" },
      { 0x0100, "0x0100 - Left Shoulder" },
      { 0x0200, "0x0200 - Right Shoulder" },
      { 0x0300, "0x0300 - Left + Right Shoulder" },
      { 0x1000, "0x1000 - A / Cross" },
      { 0x2000, "0x2000 - B / Circle" },
      { 0x4000, "0x4000 - X / Square" },
      { 0x8000, "0x8000 - Y / Triangle" }
    };
  }

  std::vector<std::pair<DWORD, std::string>> buildKeyboardValueOptions()
  {
    return
    {
      { 0x0000, "0x0000 - None" },
      { 0x08, "0x08 - Backspace" },
      { 0x09, "0x09 - Tab" },
      { 0x11, "0x11 - Ctrl" },
      { 0x12, "0x12 - Alt" },
      { 0x1B, "0x1B - Escape" },
      { 0x20, "0x20 - Space" },
      { 0x25, "0x25 - Left Arrow" },
      { 0x26, "0x26 - Up Arrow" },
      { 0x27, "0x27 - Right Arrow" },
      { 0x28, "0x28 - Down Arrow" },
      { 0x86, "0x86 - F23" },
      { 0x87, "0x87 - F24" },
      { 0xA8, "0xA8 - Browser Back" }
    };
  }

  std::string buildMappingsHelpText()
  {
    std::ostringstream stream;
    stream << "Mapping reference\r\n\r\n";
    stream << "Mouse controls\r\n";
    stream << "- CONFIG_MOUSE_LEFT: mouse button trigger\r\n";
    stream << "- CONFIG_MOUSE_RIGHT: right click trigger\r\n";
    stream << "- CONFIG_MOUSE_MIDDLE: middle click trigger\r\n\r\n";
    stream << "Program controls\r\n";
    stream << "- CONFIG_DISABLE: toggle NexPad on or off\r\n";
    stream << "- CONFIG_SPEED_CHANGE: cycle cursor speeds\r\n";
    stream << "- CONFIG_DISABLE_VIBRATION: vibration toggle\r\n";
    stream << "- CONFIG_OSK: on-screen keyboard\r\n\r\n";
    stream << "Keyboard mappings\r\n";
    stream << "- GAMEPAD_* values map controller buttons to virtual keys\r\n";
    stream << "- ON_ENABLE / ON_DISABLE send optional keys when toggled\r\n\r\n";
    stream << "Formatting\r\n";
    stream << "- Use config.ini style values such as 0x1000 or 0\r\n";
    stream << "- Keep one KEY = VALUE entry per line\r\n";
    stream << "- Preset import/export uses the same text format\r\n";
    return stream.str();
  }

  bool loadTextFile(const char* path, std::string& content)
  {
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input)
    {
      return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    content = buffer.str();
    return true;
  }

  bool saveTextFile(const char* path, const std::string& content)
  {
    std::ofstream output(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!output)
    {
      return false;
    }

    output << content;
    return true;
  }

  std::string formatHexString(const DWORD value)
  {
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << value;
    return stream.str();
  }

  std::string buildLogTimestamp()
  {
    SYSTEMTIME localTime = {};
    GetLocalTime(&localTime);

    std::ostringstream stream;
    stream << '['
           << std::setfill('0') << std::setw(2) << localTime.wHour
           << ':'
           << std::setfill('0') << std::setw(2) << localTime.wMinute
           << ':'
           << std::setfill('0') << std::setw(2) << localTime.wSecond
           << "] ";
    return stream.str();
  }

  void ensureTrayIcon(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state || state->trayAdded)
    {
      return;
    }

    ZeroMemory(&state->trayIconData, sizeof(state->trayIconData));
    state->trayIconData.cbSize = sizeof(state->trayIconData);
    state->trayIconData.hWnd = window;
    state->trayIconData.uID = ID_TRAY_ICON;
    state->trayIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    state->trayIconData.uCallbackMessage = WMAPP_TRAYICON;
    state->trayIconData.hIcon = LoadIcon(reinterpret_cast<HINSTANCE>(GetWindowLongPtr(window, GWLP_HINSTANCE)), MAKEINTRESOURCE(IDI_ICON1));
    strcpy_s(state->trayIconData.szTip, "NexPad");

    state->trayAdded = Shell_NotifyIconA(NIM_ADD, &state->trayIconData) == TRUE;
    updateTrayTooltip(window);
  }

  void removeTrayIcon(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state || !state->trayAdded)
    {
      return;
    }

    Shell_NotifyIconA(NIM_DELETE, &state->trayIconData);
    state->trayAdded = false;
  }

  void restoreFromTray(HWND window)
  {
    removeTrayIcon(window);
    ShowWindow(window, SW_SHOW);
    ShowWindow(window, SW_RESTORE);
    SetForegroundWindow(window);
  }

  void minimizeToTray(HWND window)
  {
    ensureTrayIcon(window);
    ShowWindow(window, SW_HIDE);
  }

  void showTrayMenu(HWND window)
  {
    HMENU menu = CreatePopupMenu();
    if (menu == NULL)
    {
      return;
    }

    AppendMenu(menu, MF_STRING, ID_TRAY_RESTORE, TEXT("Restore"));
    AppendMenu(menu, MF_STRING, ID_TRAY_EXIT, TEXT("Exit"));

    POINT point;
    GetCursorPos(&point);
    SetForegroundWindow(window);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, point.x, point.y, 0, window, NULL);
    DestroyMenu(menu);
  }

  void appendOutput(HWND window, const std::string& message)
  {
    GuiState* state = getGuiState(window);
    if (!state || state->outputInfo == NULL)
    {
      return;
    }

    const std::string entry = buildLogTimestamp() + message + "\r\n";
    const int length = GetWindowTextLengthA(state->outputInfo);
    SendMessageA(state->outputInfo, EM_SETSEL, length, length);
    SendMessageA(state->outputInfo, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(entry.c_str()));
  }

  void updateTrayTooltip(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state || !state->trayAdded)
    {
      return;
    }

    std::ostringstream tooltip;
    tooltip << "NexPad - "
            << (state->nexPad.isDisabled() ? "Disabled" : "Enabled")
            << " - "
            << (state->controller.GetLastConnectionState() ? state->controller.GetControllerTypeName() : std::string("No controller"));

    std::string tooltipText = tooltip.str();
    if (state->cachedTrayTooltip == tooltipText)
    {
      return;
    }

    state->cachedTrayTooltip = tooltipText;
    strncpy_s(state->trayIconData.szTip, tooltipText.c_str(), sizeof(state->trayIconData.szTip) - 1);
    state->trayIconData.uFlags = NIF_TIP;
    Shell_NotifyIconA(NIM_MODIFY, &state->trayIconData);
    state->trayIconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  }

  void updateStatusControls(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    setTextIfChanged(state->statusText, state->cachedStatusText, std::string("Status: ") + (state->nexPad.isDisabled() ? "Disabled" : "Enabled"));
    setTextIfChanged(state->controllerText, state->cachedControllerText, std::string("Controller: ") + (state->controller.GetLastConnectionState() ? "Connected" : "Disconnected"));
    setTextIfChanged(state->controllerTypeText, state->cachedControllerTypeText, "Detected type: " + state->controller.GetControllerTypeName());
    setTextIfChanged(state->batteryText, state->cachedBatteryText, state->controller.GetBatteryStatus());

    std::ostringstream speedStream;
    speedStream << std::fixed << std::setprecision(3)
                << "Current speed: " << state->nexPad.getCurrentSpeed();
    const std::vector<std::string>& speedNames = state->nexPad.getSpeedNames();
    const unsigned int speedIndex = state->nexPad.getSpeedIndex();
    if (speedIndex < speedNames.size())
    {
      speedStream << " (" << speedNames[speedIndex] << ")";
    }
    setTextIfChanged(state->speedText, state->cachedSpeedText, speedStream.str());

    std::ostringstream scrollStream;
    scrollStream << std::fixed << std::setprecision(3)
                 << "Scroll speed: " << state->nexPad.getScrollSpeed();
    setTextIfChanged(state->scrollText, state->cachedScrollText, scrollStream.str());

    setTextIfChanged(state->configText, state->cachedConfigText, "Config: " + state->nexPad.getConfigPath());
    setTextIfChanged(state->toggleButton, state->cachedToggleButtonText, state->nexPad.isDisabled() ? "Enable NexPad" : "Disable NexPad");
    updateTrayTooltip(window);
  }

  void populateSettingsControls(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    SendMessage(state->speedCombo, CB_RESETCONTENT, 0, 0);
    const std::vector<float>& speeds = state->nexPad.getSpeeds();
    const std::vector<std::string>& speedNames = state->nexPad.getSpeedNames();
    for (size_t index = 0; index < speeds.size(); ++index)
    {
      std::ostringstream entry;
      const std::string& name = index < speedNames.size() ? speedNames[index] : std::to_string(index + 1);
      entry << name << " (" << std::fixed << std::setprecision(3) << speeds[index] << ")";
      SendMessageA(state->speedCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(entry.str().c_str()));
    }
    SendMessage(state->speedCombo, CB_SETCURSEL, state->nexPad.getSpeedIndex(), 0);
    refreshComboDisplay(state->speedCombo);

    std::ostringstream scrollSpeed;
    scrollSpeed << std::fixed << std::setprecision(3) << state->nexPad.getScrollSpeed();
    SetWindowTextA(state->scrollEdit, scrollSpeed.str().c_str());

    std::ostringstream touchpadSpeed;
    touchpadSpeed << std::fixed << std::setprecision(3) << state->nexPad.getTouchpadSpeed();
    SetWindowTextA(state->touchpadSpeedEdit, touchpadSpeed.str().c_str());

    std::ostringstream touchpadDeadZone;
    touchpadDeadZone << state->nexPad.getTouchpadDeadZone();
    SetWindowTextA(state->touchpadDeadZoneEdit, touchpadDeadZone.str().c_str());

    SendMessage(state->touchpadCheck, BM_SETCHECK, state->nexPad.getTouchpadEnabled() ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessage(state->swapCheck, BM_SETCHECK, state->nexPad.getSwapThumbsticks() ? BST_CHECKED : BST_UNCHECKED, 0);
  }

  void populatePresetList(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    SendMessage(state->presetList, CB_RESETCONTENT, 0, 0);
    const std::vector<std::string> presets = enumeratePresetFiles();
    for (const std::string& preset : presets)
    {
      SendMessageA(state->presetList, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(preset.c_str()));
    }

    if (!presets.empty())
    {
      SendMessage(state->presetList, CB_SETCURSEL, 0, 0);
    }

    SendMessage(state->presetList, CB_SETDROPPEDWIDTH, 320, 0);
    refreshComboDisplay(state->presetList);
  }

  void populateMappingEditor(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    SendMessage(state->mappingKeyCombo, CB_RESETCONTENT, 0, 0);
    const std::vector<NexPad::MappingEntry> mappings = state->nexPad.getMappingEntries();
    for (const NexPad::MappingEntry& mapping : mappings)
    {
      std::string label = mapping.key + " - " + mapping.description;
      SendMessageA(state->mappingKeyCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(label.c_str()));
    }

    if (!mappings.empty())
    {
      SendMessage(state->mappingKeyCombo, CB_SETCURSEL, 0, 0);
    }

    SendMessage(state->mappingKeyCombo, CB_SETDROPPEDWIDTH, 640, 0);
    refreshComboDisplay(state->mappingKeyCombo);
  }

  DWORD parseMappingValueText(const std::string& text)
  {
    const size_t delimiter = text.find(' ');
    const std::string numeric = delimiter == std::string::npos ? text : text.substr(0, delimiter);
    return static_cast<DWORD>(strtoul(numeric.c_str(), 0, 0));
  }

  void updateSelectedMappingControls(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    const LRESULT selectedIndex = SendMessage(state->mappingKeyCombo, CB_GETCURSEL, 0, 0);
    const std::vector<NexPad::MappingEntry> mappings = state->nexPad.getMappingEntries();
    if (selectedIndex == CB_ERR || static_cast<size_t>(selectedIndex) >= mappings.size())
    {
      return;
    }

    const NexPad::MappingEntry& mapping = mappings[static_cast<size_t>(selectedIndex)];
    SetWindowTextA(state->mappingDescription, (mapping.description + (mapping.keyboardValue ? " (keyboard value)" : " (gamepad combo value)")).c_str());

    SendMessage(state->mappingValueCombo, CB_RESETCONTENT, 0, 0);
    const std::vector<std::pair<DWORD, std::string>> options = mapping.keyboardValue ? buildKeyboardValueOptions() : buildGamepadValueOptions();
    int currentSelection = -1;
    for (size_t index = 0; index < options.size(); ++index)
    {
      SendMessageA(state->mappingValueCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(options[index].second.c_str()));
      if (options[index].first == mapping.value)
      {
        currentSelection = static_cast<int>(index);
      }
    }

    if (currentSelection >= 0)
    {
      SendMessage(state->mappingValueCombo, CB_SETCURSEL, currentSelection, 0);
    }
    else
    {
      SetWindowTextA(state->mappingValueCombo, formatHexString(mapping.value).c_str());
    }

    SendMessage(state->mappingValueCombo, CB_SETDROPPEDWIDTH, 640, 0);
    refreshComboDisplay(state->mappingValueCombo);
  }

  void showSelectedTab(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    const int selectedTab = TabCtrl_GetCurSel(state->tab);
    ShowWindow(state->statusPage, selectedTab == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(state->settingsPage, selectedTab == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(state->mappingsPage, selectedTab == 2 ? SW_SHOW : SW_HIDE);
  }

  void layoutControls(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    RECT clientRect;
    GetClientRect(window, &clientRect);
    MoveWindow(state->tab, 12, 12, clientRect.right - 24, clientRect.bottom - 24, TRUE);

    RECT tabRect;
    GetClientRect(state->tab, &tabRect);
    TabCtrl_AdjustRect(state->tab, FALSE, &tabRect);

    MoveWindow(state->statusPage, tabRect.left, tabRect.top, tabRect.right - tabRect.left, tabRect.bottom - tabRect.top, TRUE);
    MoveWindow(state->settingsPage, tabRect.left, tabRect.top, tabRect.right - tabRect.left, tabRect.bottom - tabRect.top, TRUE);
    MoveWindow(state->mappingsPage, tabRect.left, tabRect.top, tabRect.right - tabRect.left, tabRect.bottom - tabRect.top, TRUE);

    const int pageWidth = tabRect.right - tabRect.left;
    const int pageHeight = tabRect.bottom - tabRect.top;
    const int margin = 12;
    const int labelHeight = 22;
    const int buttonWidth = 140;
    const int helpWidth = 180;
    const int gap = 12;

    MoveWindow(state->statusText, margin, margin, pageWidth - margin * 2 - buttonWidth - 8, labelHeight, TRUE);
    MoveWindow(state->toggleButton, pageWidth - margin - buttonWidth, margin - 2, buttonWidth, 28, TRUE);
    MoveWindow(state->controllerText, margin, margin + 28, pageWidth - margin * 2, labelHeight, TRUE);
    MoveWindow(state->controllerTypeText, margin, margin + 56, pageWidth - margin * 2, labelHeight, TRUE);
    MoveWindow(state->batteryText, margin, margin + 84, pageWidth - margin * 2, labelHeight, TRUE);
    MoveWindow(state->speedText, margin, margin + 112, pageWidth - margin * 2, labelHeight, TRUE);
    MoveWindow(state->scrollText, margin, margin + 140, pageWidth - margin * 2, labelHeight, TRUE);
    MoveWindow(state->configText, margin, margin + 168, pageWidth - margin * 2, labelHeight, TRUE);

    const int textTop = margin + 200;
    const int textHeight = (pageHeight - textTop - margin * 2) / 2;
    MoveWindow(state->startupInfo, margin, textTop, pageWidth - margin * 2, textHeight, TRUE);
    MoveWindow(state->outputInfo, margin, textTop + textHeight + margin, pageWidth - margin * 2, pageHeight - textTop - textHeight - margin * 2, TRUE);
    applyEditPadding(state->startupInfo);
    applyEditPadding(state->outputInfo);

    MoveWindow(state->speedLabel, margin, margin, 160, labelHeight, TRUE);
    MoveWindow(state->speedCombo, margin, margin + 20, pageWidth - margin * 2, 300, TRUE);
    MoveWindow(state->scrollLabel, margin, margin + 56, 160, labelHeight, TRUE);
    MoveWindow(state->scrollEdit, margin, margin + 76, 120, 24, TRUE);
    applyEditPadding(state->scrollEdit, 6, 0);
    MoveWindow(state->touchpadSpeedLabel, margin, margin + 112, 160, labelHeight, TRUE);
    MoveWindow(state->touchpadSpeedEdit, margin, margin + 132, 120, 24, TRUE);
    applyEditPadding(state->touchpadSpeedEdit, 6, 0);
    MoveWindow(state->touchpadDeadZoneLabel, margin, margin + 168, 160, labelHeight, TRUE);
    MoveWindow(state->touchpadDeadZoneEdit, margin, margin + 188, 120, 24, TRUE);
    applyEditPadding(state->touchpadDeadZoneEdit, 6, 0);
    MoveWindow(state->touchpadCheck, margin, margin + 224, pageWidth - margin * 2, 24, TRUE);
    MoveWindow(state->swapCheck, margin, margin + 256, pageWidth - margin * 2, 24, TRUE);
    MoveWindow(state->applyButton, margin, margin + 296, 130, 28, TRUE);
    MoveWindow(state->saveButton, margin + 142, margin + 296, 130, 28, TRUE);
    MoveWindow(state->reloadButton, margin + 284, margin + 296, 130, 28, TRUE);
    MoveWindow(state->presetListLabel, margin, margin + 350, 160, labelHeight, TRUE);
    MoveWindow(state->presetList, margin, margin + 370, 260, 220, TRUE);
    MoveWindow(state->presetNameLabel, margin + 272, margin + 350, 160, labelHeight, TRUE);
    MoveWindow(state->presetNameEdit, margin + 272, margin + 370, 220, 24, TRUE);
    applyEditPadding(state->presetNameEdit, 6, 0);
    MoveWindow(state->presetSaveButton, margin + 504, margin + 368, 110, 28, TRUE);
    MoveWindow(state->presetRefreshButton, margin + 272, margin + 404, 110, 28, TRUE);
    MoveWindow(state->presetDeleteButton, margin + 394, margin + 404, 110, 28, TRUE);
    MoveWindow(state->importButton, margin + 272, margin + 438, 110, 28, TRUE);
    MoveWindow(state->exportButton, margin + 394, margin + 438, 110, 28, TRUE);
    MoveWindow(state->settingsNote, margin, margin + 484, pageWidth - margin * 2, 36, TRUE);

    MoveWindow(state->mappingsHelp, margin, margin, helpWidth, pageHeight - margin * 2, TRUE);
    applyEditPadding(state->mappingsHelp);

    const int mappingsLeft = margin + helpWidth + gap;
    const int mappingsWidth = pageWidth - mappingsLeft - margin;
    const int mappingsComboGap = 12;
    const int mappingKeyWidth = (mappingsWidth - mappingsComboGap) / 2;
    const int mappingValueWidth = mappingsWidth - mappingKeyWidth - mappingsComboGap;

    MoveWindow(state->mappingKeyCombo, mappingsLeft, margin, mappingKeyWidth, 400, TRUE);
    MoveWindow(state->mappingValueCombo, mappingsLeft + mappingKeyWidth + mappingsComboGap, margin, mappingValueWidth, 400, TRUE);
    MoveWindow(state->mappingDescription, mappingsLeft, margin + 34, mappingsWidth, 22, TRUE);
    MoveWindow(state->applyMappingButton, mappingsLeft, margin + 64, 140, 28, TRUE);
  }

  void applySettings(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    const LRESULT selectedIndex = SendMessage(state->speedCombo, CB_GETCURSEL, 0, 0);
    if (selectedIndex != CB_ERR)
    {
      state->nexPad.setSpeedIndex(static_cast<unsigned int>(selectedIndex));
    }

    char scrollBuffer[64] = {};
    GetWindowTextA(state->scrollEdit, scrollBuffer, static_cast<int>(sizeof(scrollBuffer)));
    const float scrollSpeed = static_cast<float>(atof(scrollBuffer));
    if (scrollSpeed > 0.0f)
    {
      state->nexPad.setScrollSpeed(scrollSpeed);
    }

    char touchpadBuffer[64] = {};
    GetWindowTextA(state->touchpadSpeedEdit, touchpadBuffer, static_cast<int>(sizeof(touchpadBuffer)));
    const float touchpadSpeed = static_cast<float>(atof(touchpadBuffer));
    if (touchpadSpeed > 0.0f)
    {
      state->nexPad.setTouchpadSpeed(touchpadSpeed);
    }

    char touchpadDeadZoneBuffer[64] = {};
    GetWindowTextA(state->touchpadDeadZoneEdit, touchpadDeadZoneBuffer, static_cast<int>(sizeof(touchpadDeadZoneBuffer)));
    if (touchpadDeadZoneBuffer[0] != '\0')
    {
      const int touchpadDeadZone = static_cast<int>(strtol(touchpadDeadZoneBuffer, 0, 0));
      if (touchpadDeadZone >= 0)
      {
        state->nexPad.setTouchpadDeadZone(touchpadDeadZone);
      }
    }

    state->nexPad.setTouchpadEnabled(SendMessage(state->touchpadCheck, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0);
    state->nexPad.setSwapThumbsticks(SendMessage(state->swapCheck, BM_GETCHECK, 0, 0) == BST_CHECKED ? 1 : 0);
    state->nexPad.saveConfigFile();
    updateStatusControls(window);
    appendOutput(window, "Applied and auto-saved live settings.");
  }

  void reloadConfig(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    state->nexPad.loadConfigFile();
    populateSettingsControls(window);
    updateStatusControls(window);
    appendOutput(window, "Reloaded configuration from disk.");
  }

  void saveSettings(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    applySettings(window);
    if (state->nexPad.saveConfigFile())
    {
      appendOutput(window, "Saved settings to config.ini.");
    }
  }

  std::string getSelectedPresetName(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return std::string();
    }

    char buffer[MAX_PATH] = {};
    const int selectedIndex = static_cast<int>(SendMessage(state->presetList, CB_GETCURSEL, 0, 0));
    if (selectedIndex != CB_ERR)
    {
      SendMessageA(state->presetList, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(buffer));
      return buffer;
    }

    GetWindowTextA(state->presetNameEdit, buffer, MAX_PATH);
    return buffer;
  }

  void refreshPresetControls(HWND window)
  {
    populatePresetList(window);
  }

  void savePresetToList(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    char nameBuffer[MAX_PATH] = {};
    GetWindowTextA(state->presetNameEdit, nameBuffer, MAX_PATH);
    std::string presetName = nameBuffer;
    if (presetName.empty())
    {
      MessageBoxA(window, "Enter a preset name first.", "NexPad", MB_OK | MB_ICONINFORMATION);
      return;
    }

    if (presetName.find('.') == std::string::npos)
    {
      presetName += ".ini";
    }

    ensurePresetDirectory();
    const std::string presetPath = getPresetDirectory() + "\\" + presetName;
    if (!saveTextFile(presetPath.c_str(), state->nexPad.getProfileText()))
    {
      MessageBoxA(window, "Unable to save the preset file.", "NexPad", MB_OK | MB_ICONERROR);
      return;
    }

    refreshPresetControls(window);
    appendOutput(window, std::string("Saved preset ") + presetName);
  }

  void loadSelectedPreset(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    const std::string presetName = getSelectedPresetName(window);
    if (presetName.empty())
    {
      MessageBoxA(window, "Select a preset to load.", "NexPad", MB_OK | MB_ICONINFORMATION);
      return;
    }

    std::string profileText;
    if (!loadTextFile((getPresetDirectory() + "\\" + presetName).c_str(), profileText) || !state->nexPad.applyProfileText(profileText))
    {
      MessageBoxA(window, "Unable to load the selected preset.", "NexPad", MB_OK | MB_ICONWARNING);
      return;
    }

    state->nexPad.saveConfigFile();
    populateSettingsControls(window);
    populateMappingEditor(window);
    updateSelectedMappingControls(window);
    updateStatusControls(window);
    appendOutput(window, std::string("Loaded preset ") + presetName);
  }

  void deleteSelectedPreset(HWND window)
  {
    const std::string presetName = getSelectedPresetName(window);
    if (presetName.empty())
    {
      MessageBoxA(window, "Select a preset to delete.", "NexPad", MB_OK | MB_ICONINFORMATION);
      return;
    }

    const std::string presetPath = getPresetDirectory() + "\\" + presetName;
    if (!DeleteFileA(presetPath.c_str()))
    {
      MessageBoxA(window, "Unable to delete the selected preset.", "NexPad", MB_OK | MB_ICONERROR);
      return;
    }

    refreshPresetControls(window);
    appendOutput(window, std::string("Deleted preset ") + presetName);
  }

  void applySelectedMapping(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    const LRESULT selectedIndex = SendMessage(state->mappingKeyCombo, CB_GETCURSEL, 0, 0);
    const std::vector<NexPad::MappingEntry> mappings = state->nexPad.getMappingEntries();
    if (selectedIndex == CB_ERR || static_cast<size_t>(selectedIndex) >= mappings.size())
    {
      return;
    }

    char valueBuffer[128] = {};
    GetWindowTextA(state->mappingValueCombo, valueBuffer, sizeof(valueBuffer));
    const DWORD value = parseMappingValueText(valueBuffer);
    if (!state->nexPad.setMappingValue(mappings[static_cast<size_t>(selectedIndex)].key, value))
    {
      MessageBoxA(window, "Unable to update the selected mapping.", "NexPad", MB_OK | MB_ICONWARNING);
      return;
    }

    state->nexPad.saveConfigFile();
    populateMappingEditor(window);
    SendMessage(state->mappingKeyCombo, CB_SETCURSEL, selectedIndex, 0);
    refreshComboDisplay(state->mappingKeyCombo);
    updateSelectedMappingControls(window);
    appendOutput(window, std::string("Updated mapping ") + mappings[static_cast<size_t>(selectedIndex)].key + " to " + formatHexString(value));
  }

  void importPreset(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    char filePath[MAX_PATH] = {};
    OPENFILENAMEA dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = window;
    dialog.lpstrFilter = "NexPad preset (*.ini;*.txt)\0*.ini;*.txt\0All files\0*.*\0";
    dialog.lpstrFile = filePath;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    dialog.lpstrDefExt = "ini";

    if (!GetOpenFileNameA(&dialog))
    {
      return;
    }

    std::string profileText;
    if (!loadTextFile(filePath, profileText))
    {
      MessageBoxA(window, "Unable to read the selected preset file.", "NexPad", MB_OK | MB_ICONERROR);
      return;
    }

    if (!state->nexPad.applyProfileText(profileText))
    {
      MessageBoxA(window, "The selected preset file is invalid or incomplete.", "NexPad", MB_OK | MB_ICONWARNING);
      return;
    }

    state->nexPad.saveConfigFile();
    populateSettingsControls(window);
    populatePresetList(window);
    populateMappingEditor(window);
    updateSelectedMappingControls(window);
    updateStatusControls(window);
    appendOutput(window, std::string("Imported preset profile from ") + filePath);
  }

  void exportPreset(HWND window)
  {
    GuiState* state = getGuiState(window);
    if (!state)
    {
      return;
    }

    char filePath[MAX_PATH] = "NexPad-profile.ini";
    OPENFILENAMEA dialog = {};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = window;
    dialog.lpstrFilter = "NexPad preset (*.ini)\0*.ini\0Text file (*.txt)\0*.txt\0All files\0*.*\0";
    dialog.lpstrFile = filePath;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    dialog.lpstrDefExt = "ini";

    if (!GetSaveFileNameA(&dialog))
    {
      return;
    }

    if (!saveTextFile(filePath, state->nexPad.getProfileText()))
    {
      MessageBoxA(window, "Unable to write the preset file.", "NexPad", MB_OK | MB_ICONERROR);
      return;
    }

    appendOutput(window, std::string("Exported preset profile to ") + filePath);
  }

  LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
  {
    GuiState* state = getGuiState(window);

    switch (message)
    {
    case WM_CREATE:
      {
        CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        state = reinterpret_cast<GuiState*>(createStruct->lpCreateParams);
        SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));

        state->font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        state->tab = CreateWindowExA(0, WC_TABCONTROLA, "", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TCS_OWNERDRAWFIXED | TCS_FIXEDWIDTH,
                                    0, 0, 0, 0, window, reinterpret_cast<HMENU>(IDC_MAIN_TAB), createStruct->hInstance, NULL);
        SetWindowSubclass(state->tab, tabSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
        state->statusPage = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
                                           0, 0, 0, 0, state->tab, NULL, createStruct->hInstance, NULL);
        state->settingsPage = CreateWindowExA(0, "STATIC", "", WS_CHILD,
                                             0, 0, 0, 0, state->tab, NULL, createStruct->hInstance, NULL);
        state->mappingsPage = CreateWindowExA(0, "STATIC", "", WS_CHILD,
                                              0, 0, 0, 0, state->tab, NULL, createStruct->hInstance, NULL);
        SetWindowSubclass(state->statusPage, pageSubclassProc, 1, 0);
        SetWindowSubclass(state->settingsPage, pageSubclassProc, 1, 0);
        SetWindowSubclass(state->mappingsPage, pageSubclassProc, 1, 0);

        state->statusText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
                                           0, 0, 0, 0, state->statusPage, reinterpret_cast<HMENU>(IDC_STATUS_TEXT), createStruct->hInstance, NULL);
        state->controllerText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
                                                0, 0, 0, 0, state->statusPage, reinterpret_cast<HMENU>(IDC_CONTROLLER_TEXT), createStruct->hInstance, NULL);
        state->controllerTypeText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
                                                    0, 0, 0, 0, state->statusPage, reinterpret_cast<HMENU>(IDC_CONTROLLER_TYPE_TEXT), createStruct->hInstance, NULL);
        state->batteryText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
                                             0, 0, 0, 0, state->statusPage, reinterpret_cast<HMENU>(IDC_BATTERY_TEXT), createStruct->hInstance, NULL);
        state->speedText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
                                          0, 0, 0, 0, state->statusPage, reinterpret_cast<HMENU>(IDC_SPEED_TEXT), createStruct->hInstance, NULL);
        state->scrollText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
                                           0, 0, 0, 0, state->statusPage, reinterpret_cast<HMENU>(IDC_SCROLL_TEXT), createStruct->hInstance, NULL);
        state->configText = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
                                           0, 0, 0, 0, state->statusPage, reinterpret_cast<HMENU>(IDC_CONFIG_TEXT), createStruct->hInstance, NULL);
        state->toggleButton = CreateWindowExA(0, "BUTTON", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                              0, 0, 0, 0, state->statusPage, reinterpret_cast<HMENU>(IDC_TOGGLE_BUTTON), createStruct->hInstance, NULL);
        state->startupInfo = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                                             0, 0, 0, 0, state->statusPage, reinterpret_cast<HMENU>(IDC_STARTUP_INFO), createStruct->hInstance, NULL);
        state->outputInfo = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                                            0, 0, 0, 0, state->statusPage, reinterpret_cast<HMENU>(IDC_OUTPUT_INFO), createStruct->hInstance, NULL);

        state->speedLabel = CreateWindowExA(0, "STATIC", "Speed preset", WS_CHILD | WS_VISIBLE,
                 12, 12, 160, 20, state->settingsPage, NULL, createStruct->hInstance, NULL);
        state->speedCombo = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | (kUseNativeComboPrototype ? CBS_DROPDOWN : (CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS)) | WS_VSCROLL,
                                            0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_SPEED_COMBO), createStruct->hInstance, NULL);
        state->scrollLabel = CreateWindowExA(0, "STATIC", "Scroll speed", WS_CHILD | WS_VISIBLE,
                 12, 68, 160, 20, state->settingsPage, NULL, createStruct->hInstance, NULL);
        state->scrollEdit = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                                            0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_SCROLL_EDIT), createStruct->hInstance, NULL);
        state->touchpadSpeedLabel = CreateWindowExA(0, "STATIC", "Touchpad speed", WS_CHILD | WS_VISIBLE,
           12, 104, 160, 20, state->settingsPage, NULL, createStruct->hInstance, NULL);
        state->touchpadSpeedEdit = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                     0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_TOUCHPAD_SPEED_EDIT), createStruct->hInstance, NULL);
          state->touchpadDeadZoneLabel = CreateWindowExA(0, "STATIC", "Touchpad dead zone", WS_CHILD | WS_VISIBLE,
            12, 160, 160, 20, state->settingsPage, NULL, createStruct->hInstance, NULL);
          state->touchpadDeadZoneEdit = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                   0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_TOUCHPAD_DEAD_ZONE_EDIT), createStruct->hInstance, NULL);
        state->touchpadCheck = CreateWindowExA(0, "BUTTON", "Enable DualSense touchpad", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                       0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_TOUCHPAD_CHECK), createStruct->hInstance, NULL);
        state->swapCheck = CreateWindowExA(0, "BUTTON", "Swap thumbsticks", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                          0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_SWAP_CHECK), createStruct->hInstance, NULL);
        state->presetListLabel = CreateWindowExA(0, "STATIC", "Preset list", WS_CHILD | WS_VISIBLE,
                        12, 292, 160, 20, state->settingsPage, NULL, createStruct->hInstance, NULL);
        state->presetList = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | (kUseNativeComboPrototype ? CBS_DROPDOWN : (CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS)) | WS_VSCROLL,
                                            0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_PRESET_LIST), createStruct->hInstance, NULL);
        state->presetNameLabel = CreateWindowExA(0, "STATIC", "Preset name", WS_CHILD | WS_VISIBLE,
                        284, 292, 160, 20, state->settingsPage, NULL, createStruct->hInstance, NULL);
        state->presetNameEdit = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
                                                0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_PRESET_NAME_EDIT), createStruct->hInstance, NULL);
        state->applyButton = CreateWindowExA(0, "BUTTON", "Apply", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                            0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_APPLY_BUTTON), createStruct->hInstance, NULL);
        state->saveButton = CreateWindowExA(0, "BUTTON", "Save to config", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                            0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_SAVE_BUTTON), createStruct->hInstance, NULL);
        state->reloadButton = CreateWindowExA(0, "BUTTON", "Reload config", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                             0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_RELOAD_BUTTON), createStruct->hInstance, NULL);
        state->presetRefreshButton = CreateWindowExA(0, "BUTTON", "Refresh list", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                                     0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_PRESET_REFRESH_BUTTON), createStruct->hInstance, NULL);
        state->presetSaveButton = CreateWindowExA(0, "BUTTON", "Save preset", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                                  0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_PRESET_SAVE_BUTTON), createStruct->hInstance, NULL);
        state->presetDeleteButton = CreateWindowExA(0, "BUTTON", "Delete preset", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                                    0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_PRESET_DELETE_BUTTON), createStruct->hInstance, NULL);
        state->importButton = CreateWindowExA(0, "BUTTON", "Import preset", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                              0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_IMPORT_BUTTON), createStruct->hInstance, NULL);
        state->exportButton = CreateWindowExA(0, "BUTTON", "Export preset", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                              0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_EXPORT_BUTTON), createStruct->hInstance, NULL);
        state->settingsNote = CreateWindowExA(0, "STATIC", buildPresetHelpText().c_str(), WS_CHILD | WS_VISIBLE,
                                             0, 0, 0, 0, state->settingsPage, reinterpret_cast<HMENU>(IDC_SETTINGS_NOTE), createStruct->hInstance, NULL);
        state->mappingsHelp = CreateWindowExA(0, "EDIT", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                                              0, 0, 0, 0, state->mappingsPage, reinterpret_cast<HMENU>(IDC_MAPPINGS_HELP), createStruct->hInstance, NULL);
        state->mappingKeyCombo = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | (kUseNativeComboPrototype ? CBS_DROPDOWN : (CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS)) | WS_VSCROLL,
                                                 0, 0, 0, 0, state->mappingsPage, reinterpret_cast<HMENU>(IDC_MAPPING_KEY_COMBO), createStruct->hInstance, NULL);
        state->mappingValueCombo = CreateWindowExA(0, "COMBOBOX", "", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWN | (kUseNativeComboPrototype ? 0 : (CBS_OWNERDRAWFIXED | CBS_HASSTRINGS)) | WS_VSCROLL,
                                                   0, 0, 0, 0, state->mappingsPage, reinterpret_cast<HMENU>(IDC_MAPPING_VALUE_COMBO), createStruct->hInstance, NULL);
        state->mappingDescription = CreateWindowExA(0, "STATIC", "", WS_CHILD | WS_VISIBLE,
                                                    0, 0, 0, 0, state->mappingsPage, reinterpret_cast<HMENU>(IDC_MAPPING_DESC_TEXT), createStruct->hInstance, NULL);
        state->applyMappingButton = CreateWindowExA(0, "BUTTON", "Apply mapping", WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
                                                    0, 0, 0, 0, state->mappingsPage, reinterpret_cast<HMENU>(IDC_APPLY_MAPPING_BUTTON), createStruct->hInstance, NULL);

        TCITEMA tabItem = {};
        tabItem.mask = TCIF_TEXT;
        tabItem.pszText = const_cast<char*>("Status");
        SendMessageA(state->tab, TCM_INSERTITEMA, 0, reinterpret_cast<LPARAM>(&tabItem));
        tabItem.pszText = const_cast<char*>("Settings");
        SendMessageA(state->tab, TCM_INSERTITEMA, 1, reinterpret_cast<LPARAM>(&tabItem));
        tabItem.pszText = const_cast<char*>("Mappings");
        SendMessageA(state->tab, TCM_INSERTITEMA, 2, reinterpret_cast<LPARAM>(&tabItem));
        SendMessage(state->tab, TCM_SETITEMSIZE, 0, MAKELPARAM(92, 24));

        setControlFont(state->font,
                       { state->tab, state->statusText, state->controllerText, state->controllerTypeText, state->batteryText, state->speedText, state->scrollText, state->configText,
                         state->toggleButton,
                          state->startupInfo, state->outputInfo, state->speedLabel, state->speedCombo, state->scrollLabel, state->scrollEdit, state->touchpadSpeedLabel, state->touchpadSpeedEdit, state->touchpadDeadZoneLabel, state->touchpadDeadZoneEdit,
                           state->touchpadCheck, state->swapCheck, state->presetListLabel, state->presetList, state->presetNameLabel, state->presetNameEdit, state->presetRefreshButton, state->presetSaveButton, state->presetDeleteButton,
                           state->applyButton, state->saveButton, state->reloadButton, state->importButton, state->exportButton, state->settingsNote,
                           state->mappingsHelp, state->mappingKeyCombo, state->mappingValueCombo, state->mappingDescription, state->applyMappingButton });

        SetWindowTextA(state->startupInfo, buildStartupInfo().c_str());
        SetWindowTextA(state->mappingsHelp, buildMappingsHelpText().c_str());

        state->nexPad.setWindowHandle(window);
        state->nexPad.setStatusCallback([window](const std::string& message)
        {
          appendOutput(window, message);
        });
        applyControlTheme(state->tab);
        applyNativeTabPalette(state->tab, state);
        applyControlTheme(state->toggleButton);
        applyControlTheme(state->speedCombo);
        applyControlTheme(state->touchpadCheck);
        applyControlTheme(state->swapCheck);
        applyControlTheme(state->applyButton);
        applyControlTheme(state->saveButton);
        applyControlTheme(state->reloadButton);
        applyControlTheme(state->presetRefreshButton);
        applyControlTheme(state->presetSaveButton);
        applyControlTheme(state->presetDeleteButton);
        applyControlTheme(state->importButton);
        applyControlTheme(state->exportButton);
        applyControlTheme(state->applyMappingButton);
        applyControlTheme(state->presetList);
        applyControlTheme(state->mappingKeyCombo);
        applyControlTheme(state->mappingValueCombo);
        applyControlTheme(state->startupInfo);
        applyControlTheme(state->outputInfo);
        applyControlTheme(state->scrollEdit);
        applyControlTheme(state->touchpadSpeedEdit);
        applyControlTheme(state->touchpadDeadZoneEdit);
        applyControlTheme(state->presetNameEdit);
        applyControlTheme(state->mappingsHelp);

        SetWindowSubclass(state->toggleButton, buttonSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
        SetWindowSubclass(state->touchpadCheck, buttonSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
        SetWindowSubclass(state->swapCheck, buttonSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
        SetWindowSubclass(state->applyButton, buttonSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
        SetWindowSubclass(state->saveButton, buttonSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
        SetWindowSubclass(state->reloadButton, buttonSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
        SetWindowSubclass(state->presetRefreshButton, buttonSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
        SetWindowSubclass(state->presetSaveButton, buttonSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
        SetWindowSubclass(state->presetDeleteButton, buttonSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
        SetWindowSubclass(state->importButton, buttonSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
        SetWindowSubclass(state->exportButton, buttonSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
        SetWindowSubclass(state->applyMappingButton, buttonSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
        if (!kUseNativeComboPrototype)
        {
          SetWindowSubclass(state->speedCombo, comboSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
          SetWindowSubclass(state->presetList, comboSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
          SetWindowSubclass(state->mappingKeyCombo, comboSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
          SetWindowSubclass(state->mappingValueCombo, comboSubclassProc, 1, reinterpret_cast<DWORD_PTR>(state));
        }

        applyComboChildTheme(state->speedCombo, state->font, kUseNativeComboPrototype);
        applyComboChildTheme(state->presetList, state->font, kUseNativeComboPrototype);
        applyComboChildTheme(state->mappingKeyCombo, state->font, kUseNativeComboPrototype);
        applyComboChildTheme(state->mappingValueCombo, state->font, false);
        applyThemePalette(state);
        applyNativeTabPalette(state->tab, state);
        state->nexPad.loadConfigFile();
        populateSettingsControls(window);
        populatePresetList(window);
        populateMappingEditor(window);
        updateSelectedMappingControls(window);
        updateStatusControls(window);
        appendOutput(window, "NexPad GUI initialized.");
        SetTimer(window, 1, static_cast<UINT>(state->nexPad.getLoopIntervalMs()), NULL);
        layoutControls(window);
        showSelectedTab(window);
      }
      return 0;

    case WM_NOTIFY:
      if (state && reinterpret_cast<LPNMHDR>(lParam)->idFrom == IDC_MAIN_TAB && reinterpret_cast<LPNMHDR>(lParam)->code == NM_CUSTOMDRAW && (GetWindowLongPtr(state->tab, GWL_STYLE) & TCS_OWNERDRAWFIXED) == 0)
      {
        return handleTabCustomDraw(reinterpret_cast<NMCUSTOMDRAW*>(lParam), state);
      }
      if (state && reinterpret_cast<LPNMHDR>(lParam)->idFrom == IDC_MAIN_TAB && reinterpret_cast<LPNMHDR>(lParam)->code == TCN_SELCHANGE)
      {
        showSelectedTab(window);
      }
      return 0;

    case WM_MEASUREITEM:
      if (state && reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->CtlType == ODT_TAB)
      {
        reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemHeight = 24;
        reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemWidth = 92;
        return TRUE;
      }
      if (state && (reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->CtlType == ODT_COMBOBOX ||
                    reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->CtlType == ODT_LISTBOX))
      {
        reinterpret_cast<MEASUREITEMSTRUCT*>(lParam)->itemHeight = 20;
        return TRUE;
      }
      break;

    case WM_DRAWITEM:
      if (state)
      {
        DRAWITEMSTRUCT* drawItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (drawItem->CtlType == ODT_BUTTON)
        {
          drawThemedButton(drawItem, state);
          return TRUE;
        }
        if (drawItem->CtlType == ODT_TAB)
        {
          drawThemedTabItem(drawItem, state);
          return TRUE;
        }
        if (drawItem->CtlType == ODT_COMBOBOX || drawItem->CtlType == ODT_LISTBOX)
        {
          drawThemedComboItem(drawItem, state);
          return TRUE;
        }
      }
      break;

    case WM_COMMAND:
      if (LOWORD(wParam) == ID_TRAY_RESTORE)
      {
        restoreFromTray(window);
        return 0;
      }
      if (LOWORD(wParam) == ID_TRAY_EXIT)
      {
        DestroyWindow(window);
        return 0;
      }

      switch (LOWORD(wParam))
      {
      case IDC_PRESET_LIST:
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
          const std::string presetName = getSelectedPresetName(window);
          SetWindowTextA(state->presetNameEdit, presetName.c_str());
        }
        return 0;

      case IDC_MAPPING_KEY_COMBO:
        if (HIWORD(wParam) == CBN_SELCHANGE)
        {
          updateSelectedMappingControls(window);
        }
        return 0;

      case IDC_SWAP_CHECK:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          const LRESULT checked = SendMessage(state->swapCheck, BM_GETCHECK, 0, 0) == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED;
          SendMessage(state->swapCheck, BM_SETCHECK, checked, 0);
          InvalidateRect(state->swapCheck, NULL, TRUE);
        }
        return 0;

      case IDC_TOUCHPAD_CHECK:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          const LRESULT checked = SendMessage(state->touchpadCheck, BM_GETCHECK, 0, 0) == BST_CHECKED ? BST_UNCHECKED : BST_CHECKED;
          SendMessage(state->touchpadCheck, BM_SETCHECK, checked, 0);
          InvalidateRect(state->touchpadCheck, NULL, TRUE);
        }
        return 0;

      case IDC_TOGGLE_BUTTON:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          state->nexPad.setDisabled(!state->nexPad.isDisabled());
          updateStatusControls(window);
        }
        return 0;

      case IDC_APPLY_BUTTON:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          applySettings(window);
        }
        return 0;

      case IDC_SAVE_BUTTON:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          saveSettings(window);
        }
        return 0;

      case IDC_RELOAD_BUTTON:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          reloadConfig(window);
        }
        return 0;

      case IDC_IMPORT_BUTTON:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          importPreset(window);
        }
        return 0;

      case IDC_EXPORT_BUTTON:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          exportPreset(window);
        }
        return 0;

      case IDC_PRESET_REFRESH_BUTTON:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          refreshPresetControls(window);
        }
        return 0;

      case IDC_PRESET_SAVE_BUTTON:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          savePresetToList(window);
        }
        return 0;

      case IDC_PRESET_DELETE_BUTTON:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          deleteSelectedPreset(window);
        }
        return 0;

      case IDC_APPLY_MAPPING_BUTTON:
        if (HIWORD(wParam) == BN_CLICKED)
        {
          applySelectedMapping(window);
        }
        return 0;
      }
      break;

    case WM_SIZE:
      if (wParam == SIZE_MINIMIZED)
      {
        minimizeToTray(window);
        return 0;
      }

      layoutControls(window);
      return 0;

    case WMAPP_TRAYICON:
      switch (LOWORD(lParam))
      {
      case WM_LBUTTONDBLCLK:
        restoreFromTray(window);
        return 0;

      case WM_RBUTTONUP:
      case WM_CONTEXTMENU:
        showTrayMenu(window);
        return 0;
      }
      return 0;

    case WM_TIMER:
      if (state)
      {
        state->nexPad.loop();
        updateStatusControls(window);
      }
      return 0;

    case WM_DESTROY:
      KillTimer(window, 1);
      removeTrayIcon(window);
      if (state)
      {
        RemoveWindowSubclass(state->tab, tabSubclassProc, 1);
        RemoveWindowSubclass(state->statusPage, pageSubclassProc, 1);
        RemoveWindowSubclass(state->settingsPage, pageSubclassProc, 1);
        RemoveWindowSubclass(state->mappingsPage, pageSubclassProc, 1);
        RemoveWindowSubclass(state->toggleButton, buttonSubclassProc, 1);
        RemoveWindowSubclass(state->swapCheck, buttonSubclassProc, 1);
        RemoveWindowSubclass(state->applyButton, buttonSubclassProc, 1);
        RemoveWindowSubclass(state->saveButton, buttonSubclassProc, 1);
        RemoveWindowSubclass(state->reloadButton, buttonSubclassProc, 1);
        RemoveWindowSubclass(state->presetRefreshButton, buttonSubclassProc, 1);
        RemoveWindowSubclass(state->presetSaveButton, buttonSubclassProc, 1);
        RemoveWindowSubclass(state->presetDeleteButton, buttonSubclassProc, 1);
        RemoveWindowSubclass(state->importButton, buttonSubclassProc, 1);
        RemoveWindowSubclass(state->exportButton, buttonSubclassProc, 1);
        RemoveWindowSubclass(state->applyMappingButton, buttonSubclassProc, 1);
        if (!kUseNativeComboPrototype)
        {
          RemoveWindowSubclass(state->speedCombo, comboSubclassProc, 1);
          RemoveWindowSubclass(state->presetList, comboSubclassProc, 1);
          RemoveWindowSubclass(state->mappingKeyCombo, comboSubclassProc, 1);
          RemoveWindowSubclass(state->mappingValueCombo, comboSubclassProc, 1);

          COMBOBOXINFO speedInfo = {};
          speedInfo.cbSize = sizeof(speedInfo);
          if (GetComboBoxInfo(state->speedCombo, &speedInfo) && speedInfo.hwndList != NULL)
          {
            RemoveWindowSubclass(speedInfo.hwndList, comboListSubclassProc, 1);
          }

          COMBOBOXINFO presetInfo = {};
          presetInfo.cbSize = sizeof(presetInfo);
          if (GetComboBoxInfo(state->presetList, &presetInfo) && presetInfo.hwndList != NULL)
          {
            RemoveWindowSubclass(presetInfo.hwndList, comboListSubclassProc, 1);
          }

          COMBOBOXINFO mappingKeyInfo = {};
          mappingKeyInfo.cbSize = sizeof(mappingKeyInfo);
          if (GetComboBoxInfo(state->mappingKeyCombo, &mappingKeyInfo) && mappingKeyInfo.hwndList != NULL)
          {
            RemoveWindowSubclass(mappingKeyInfo.hwndList, comboListSubclassProc, 1);
          }

          COMBOBOXINFO mappingValueInfo = {};
          mappingValueInfo.cbSize = sizeof(mappingValueInfo);
          if (GetComboBoxInfo(state->mappingValueCombo, &mappingValueInfo) && mappingValueInfo.hwndList != NULL)
          {
            RemoveWindowSubclass(mappingValueInfo.hwndList, comboListSubclassProc, 1);
          }
        }
      }
      destroyThemeResources(state);
      delete state;
      PostQuitMessage(0);
      return 0;

    case WM_CTLCOLORSTATIC:
      if (state)
      {
        HDC dc = reinterpret_cast<HDC>(wParam);

        if (isEditControl(reinterpret_cast<HWND>(lParam)))
        {
          SetBkMode(dc, OPAQUE);
          SetTextColor(dc, state->textColor);
          SetBkColor(dc, state->editBackgroundColor);
          return reinterpret_cast<LRESULT>(state->editBrush);
        }

        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, state->textColor);
        SetBkColor(dc, state->panelColor);
        return reinterpret_cast<LRESULT>(state->panelBrush);
      }
      break;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
      if (state)
      {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, state->textColor);
        SetBkColor(dc, state->editBackgroundColor);
        return reinterpret_cast<LRESULT>(state->editBrush);
      }
      break;

    case WM_CTLCOLORBTN:
      if (state)
      {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, state->buttonTextColor);
        SetBkColor(dc, state->buttonColor);
        return reinterpret_cast<LRESULT>(state->buttonBrush);
      }
      break;

    case WM_ERASEBKGND:
      if (state && state->backgroundBrush != NULL)
      {
        RECT clientRect;
        GetClientRect(window, &clientRect);
        FillRect(reinterpret_cast<HDC>(wParam), &clientRect, state->backgroundBrush);
        return 1;
      }
      break;
    }

    return DefWindowProc(window, message, wParam, lParam);
  }
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
  INITCOMMONCONTROLSEX controls = {};
  controls.dwSize = sizeof(controls);
  controls.dwICC = ICC_TAB_CLASSES;
  InitCommonControlsEx(&controls);

  WNDCLASSEX windowClass = {};
  windowClass.cbSize = sizeof(windowClass);
  windowClass.lpfnWndProc = windowProc;
  windowClass.hInstance = instance;
  windowClass.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_ICON1));
  windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
  windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  windowClass.lpszClassName = TEXT("NexPadWindowClass");
  windowClass.hIconSm = LoadIcon(instance, MAKEINTRESOURCE(IDI_ICON1));
  RegisterClassEx(&windowClass);

  GuiState* state = new GuiState();
  HWND window = CreateWindowEx(0,
                               windowClass.lpszClassName,
                               TEXT("NexPad"),
                               WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT,
                               CW_USEDEFAULT,
                               720,
                               560,
                               NULL,
                               NULL,
                               instance,
                               state);
  if (window == NULL)
  {
    delete state;
    return 0;
  }

  ShowWindow(window, showCommand);
  UpdateWindow(window);

  MSG message = {};
  while (GetMessage(&message, NULL, 0, 0) > 0)
  {
    TranslateMessage(&message);
    DispatchMessage(&message);
  }

  return static_cast<int>(message.wParam);
}

BOOL isRunningAsAdministrator()
{
  BOOL   fRet = FALSE;
  HANDLE hToken = NULL;

  if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
  {
    TOKEN_ELEVATION Elevation;
    DWORD cbSize = sizeof( TOKEN_ELEVATION );

    if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof( Elevation), &cbSize))
    {
      fRet = Elevation.TokenIsElevated;
    }
  }

  if (hToken)
  {
    CloseHandle(hToken);
  }

  return fRet;
}

// This works, but it's not enabled in the software since the best button for it is still undecided
bool ChangeVolume(double nVolume, bool bScalar) //o b
{
  HRESULT hr = NULL;
  bool decibels = false;
  bool scalar = false;
  double newVolume = nVolume;

  CoInitialize(NULL);
  IMMDeviceEnumerator *deviceEnumerator = NULL;
  hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER,
                        __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
  IMMDevice *defaultDevice = NULL;

  hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
  deviceEnumerator->Release();
  deviceEnumerator = NULL;

  IAudioEndpointVolume *endpointVolume = NULL;
  hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume),
                               CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&endpointVolume);
  defaultDevice->Release();
  defaultDevice = NULL;

  // -------------------------
//   float currentVolume = 0;
//   endpointVolume->GetMasterVolumeLevel(&currentVolume);
//   //printf("Current volume in dB is: %f\n", currentVolume);

//   hr = endpointVolume->GetMasterVolumeLevelScalar(&currentVolume);
//   //CString strCur=L"";
//   //strCur.Format(L"%f",currentVolume);
//   //AfxMessageBox(strCur);

//   // printf("Current volume as a scalar is: %f\n", currentVolume);
  if (bScalar == false)
  {
    hr = endpointVolume->SetMasterVolumeLevel((float)newVolume, NULL);
  }
  else if (bScalar == true)
  {
    hr = endpointVolume->SetMasterVolumeLevelScalar((float)newVolume, NULL);
  }
  endpointVolume->Release();

  CoUninitialize();

  return FALSE;
}
