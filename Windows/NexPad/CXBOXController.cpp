#include "CXBOXController.h"

#include <setupapi.h>
#include <hidsdi.h>

#include <cwctype>
#include <vector>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "hid.lib")

namespace
{
  typedef DWORD (WINAPI *XInputGetState_t)(DWORD, XINPUT_STATE*);
  typedef DWORD (WINAPI *XInputSetState_t)(DWORD, XINPUT_VIBRATION*);
  typedef DWORD (WINAPI *XInputGetBatteryInformation_t)(DWORD, BYTE, XINPUT_BATTERY_INFORMATION*);

  XInputGetState_t g_xinputGetState = nullptr;
  XInputSetState_t g_xinputSetState = nullptr;
  XInputGetBatteryInformation_t g_xinputGetBatteryInformation = nullptr;
  bool g_xinputInitialized = false;
  std::string g_playStationBatteryStatus = "Battery: HID status unavailable";

  enum class PSControllerType
  {
    DualSense,    // PS5
    DualShock4,   // PS4
  };

  enum class PSTransportType
  {
    Unknown,
    Usb,
    Bluetooth,
  };

  struct DualSenseState
  {
    struct TouchpadFrame
    {
      bool available = false;
      bool active = false;
      unsigned short x = 0;
      unsigned short y = 0;
      unsigned short previousX = 0;
      unsigned short previousY = 0;
      short deltaX = 0;
      short deltaY = 0;
    };

    HANDLE deviceHandle = INVALID_HANDLE_VALUE;
    HANDLE readEvent = NULL;
    OVERLAPPED overlapped = {};
    USHORT inputReportLength = 64;
    DWORD lastDiscoveryTick = 0;
    bool initialized = false;
    bool readPending = false;
    bool hasCachedState = false;
    DWORD packetNumber = 0;
    std::vector<BYTE> inputBuffer;
    XINPUT_STATE cachedState = {};
    PSControllerType controllerType = PSControllerType::DualSense;
    PSTransportType transportType = PSTransportType::Unknown;
    DWORD lastValidReportTick = 0;
    TouchpadFrame touchpad = {};
  };

  DualSenseState g_dualSenseState;

  void resetTouchpadFrame();
  void clearDualSenseCachedState();

  bool isDualSenseProductId(const USHORT productId)
  {
    return productId == 0x0CE6 || // DualSense
           productId == 0x0DF2;   // DualSense Edge
  }

  bool isDualShock4ProductId(const USHORT productId)
  {
    return productId == 0x05C4 || // DualShock 4 v1
           productId == 0x09CC;   // DualShock 4 v2
  }

  bool isBluetoothDevicePath(const wchar_t* devicePath)
  {
    if (devicePath == nullptr)
    {
      return false;
    }

    std::wstring path(devicePath);
    for (wchar_t& character : path)
    {
      character = static_cast<wchar_t>(towupper(character));
    }

    return path.find(L"BTHENUM") != std::wstring::npos ||
           path.find(L"BTHLEDEVICE") != std::wstring::npos;
  }

  std::string describePlayStationBatteryLevel(const BYTE rawLevel, const bool charging)
  {
    BYTE level = rawLevel;
    if (level > 10)
    {
      level &= 0x0F;
    }

    const char* levelText = "Unknown";
    if (level == 0)
    {
      levelText = "Empty";
    }
    else if (level <= 2)
    {
      levelText = "Low";
    }
    else if (level <= 7)
    {
      levelText = "Medium";
    }
    else
    {
      levelText = "Full";
    }

    return std::string("Battery: ") + (charging ? "Charging " : "") + levelText;
  }

  void updateDualSenseBatteryStatus(const std::vector<BYTE>& report, const size_t baseOffset)
  {
    if (report.size() <= baseOffset + 52)
    {
      g_playStationBatteryStatus = "Battery: HID status unavailable";
      return;
    }

    const BYTE status = report[baseOffset + 52];
    const BYTE level = static_cast<BYTE>(status & 0x0F);
    const bool charging = (status & 0x10) != 0;
    g_playStationBatteryStatus = describePlayStationBatteryLevel(level, charging);
  }

  void updateDualShock4BatteryStatus(const std::vector<BYTE>& report, const size_t baseOffset)
  {
    if (report.size() <= baseOffset + 29)
    {
      g_playStationBatteryStatus = "Battery: HID status unavailable";
      return;
    }

    const BYTE status = report[baseOffset + 29];
    const bool charging = (status & 0x10) != 0;
    g_playStationBatteryStatus = describePlayStationBatteryLevel(status, charging);
  }

  void closeDualSenseDevice()
  {
    if (g_dualSenseState.deviceHandle != INVALID_HANDLE_VALUE)
    {
      if (g_dualSenseState.readPending)
      {
        CancelIo(g_dualSenseState.deviceHandle);
      }

      CloseHandle(g_dualSenseState.deviceHandle);
      g_dualSenseState.deviceHandle = INVALID_HANDLE_VALUE;
    }

    if (g_dualSenseState.readEvent != NULL)
    {
      CloseHandle(g_dualSenseState.readEvent);
      g_dualSenseState.readEvent = NULL;
    }

    ZeroMemory(&g_dualSenseState.overlapped, sizeof(g_dualSenseState.overlapped));
    g_dualSenseState.inputReportLength = 64;
    g_dualSenseState.readPending = false;
    g_dualSenseState.inputBuffer.clear();
    g_dualSenseState.transportType = PSTransportType::Unknown;
    clearDualSenseCachedState();
    g_playStationBatteryStatus = "Battery: Disconnected";
  }

  void resetTouchpadFrame()
  {
    g_dualSenseState.touchpad.active = false;
    g_dualSenseState.touchpad.available = false;
    g_dualSenseState.touchpad.deltaX = 0;
    g_dualSenseState.touchpad.deltaY = 0;
    g_dualSenseState.touchpad.x = 0;
    g_dualSenseState.touchpad.y = 0;
    g_dualSenseState.touchpad.previousX = 0;
    g_dualSenseState.touchpad.previousY = 0;
  }

  void clearDualSenseCachedState()
  {
    g_dualSenseState.hasCachedState = false;
    g_dualSenseState.lastValidReportTick = 0;
    ZeroMemory(&g_dualSenseState.cachedState, sizeof(g_dualSenseState.cachedState));
    resetTouchpadFrame();
  }

  void clearTouchpadDeltas()
  {
    g_dualSenseState.touchpad.deltaX = 0;
    g_dualSenseState.touchpad.deltaY = 0;
  }

  void requestDualSenseEnhancedReports(HANDLE deviceHandle)
  {
    BYTE firmwareInfoReport[64] = {};
    firmwareInfoReport[0] = 0x20;
    HidD_GetFeature(deviceHandle, firmwareInfoReport, sizeof(firmwareInfoReport));

    BYTE calibrationReport[64] = {};
    calibrationReport[0] = 0x05;
    HidD_GetFeature(deviceHandle, calibrationReport, sizeof(calibrationReport));
  }

  void updateDualSenseTouchpadState(const std::vector<BYTE>& report, const size_t baseOffset)
  {
    clearTouchpadDeltas();

    if (g_dualSenseState.controllerType != PSControllerType::DualSense)
    {
      g_dualSenseState.touchpad.available = false;
      g_dualSenseState.touchpad.active = false;
      return;
    }

    const size_t touchOffset = baseOffset + 32;
    if (report.size() <= touchOffset + 3)
    {
      g_dualSenseState.touchpad.available = false;
      g_dualSenseState.touchpad.active = false;
      return;
    }

    g_dualSenseState.touchpad.available = true;

    // TouchData starts at byte 32 of the DualSense state payload.
    // The first finger uses 4 bytes: id/inactive flag, x low, x high|y low, y high.
    const BYTE fingerState = report[touchOffset + 0];
    const bool active = (fingerState & 0x80) == 0;
    if (!active)
    {
      g_dualSenseState.touchpad.active = false;
      g_dualSenseState.touchpad.x = 0;
      g_dualSenseState.touchpad.y = 0;
      g_dualSenseState.touchpad.previousX = 0;
      g_dualSenseState.touchpad.previousY = 0;
      return;
    }

    const unsigned short touchX = static_cast<unsigned short>(report[touchOffset + 1] | ((report[touchOffset + 2] & 0x0F) << 8));
    const unsigned short touchY = static_cast<unsigned short>(((report[touchOffset + 2] >> 4) & 0x0F) | (report[touchOffset + 3] << 4));

    if (!g_dualSenseState.touchpad.active)
    {
      g_dualSenseState.touchpad.previousX = touchX;
      g_dualSenseState.touchpad.previousY = touchY;
    }
    else
    {
      g_dualSenseState.touchpad.deltaX = static_cast<short>(static_cast<int>(touchX) - static_cast<int>(g_dualSenseState.touchpad.previousX));
      g_dualSenseState.touchpad.deltaY = static_cast<short>(static_cast<int>(touchY) - static_cast<int>(g_dualSenseState.touchpad.previousY));
      g_dualSenseState.touchpad.previousX = touchX;
      g_dualSenseState.touchpad.previousY = touchY;
    }

    g_dualSenseState.touchpad.active = true;
    g_dualSenseState.touchpad.x = touchX;
    g_dualSenseState.touchpad.y = touchY;
  }

  void initializeXInput()
  {
    if (g_xinputInitialized)
    {
      return;
    }

    g_xinputInitialized = true;

    const wchar_t* xinputDlls[] =
    {
      L"xinput1_4.dll",   // Windows 8+
      L"xinput1_3.dll",   // Legacy DirectX runtime
      L"xinput9_1_0.dll"  // Vista+
    };

    for (const auto& dllName : xinputDlls)
    {
      HMODULE mod = LoadLibraryW(dllName);
      if (!mod)
      {
        continue;
      }

      XInputGetState_t getState = reinterpret_cast<XInputGetState_t>(GetProcAddress(mod, "XInputGetState"));
      XInputSetState_t setState = reinterpret_cast<XInputSetState_t>(GetProcAddress(mod, "XInputSetState"));
      XInputGetBatteryInformation_t getBatteryInformation = reinterpret_cast<XInputGetBatteryInformation_t>(GetProcAddress(mod, "XInputGetBatteryInformation"));

      if (getState && setState)
      {
        g_xinputGetState = getState;
        g_xinputSetState = setState;
        g_xinputGetBatteryInformation = getBatteryInformation;
        return;
      }

      FreeLibrary(mod);
    }
  }

  bool discoverDualSense()
  {
    const DWORD now = GetTickCount();
    if (g_dualSenseState.initialized && (now - g_dualSenseState.lastDiscoveryTick) < 1000)
    {
      return false;
    }

    g_dualSenseState.initialized = true;
    g_dualSenseState.lastDiscoveryTick = now;

    GUID hidGuid;
    HidD_GetHidGuid(&hidGuid);

    HDEVINFO hidInfo = SetupDiGetClassDevsW(&hidGuid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (hidInfo == INVALID_HANDLE_VALUE)
    {
      return false;
    }

    bool found = false;

    for (DWORD index = 0; ; ++index)
    {
      SP_DEVICE_INTERFACE_DATA interfaceData;
      ZeroMemory(&interfaceData, sizeof(interfaceData));
      interfaceData.cbSize = sizeof(interfaceData);

      if (!SetupDiEnumDeviceInterfaces(hidInfo, nullptr, &hidGuid, index, &interfaceData))
      {
        break;
      }

      DWORD requiredSize = 0;
      SetupDiGetDeviceInterfaceDetailW(hidInfo, &interfaceData, nullptr, 0, &requiredSize, nullptr);
      if (requiredSize == 0)
      {
        continue;
      }

      std::vector<BYTE> detailBuffer(requiredSize, 0);
      PSP_DEVICE_INTERFACE_DETAIL_DATA_W detailData = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(detailBuffer.data());
      detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

      if (!SetupDiGetDeviceInterfaceDetailW(hidInfo, &interfaceData, detailData, requiredSize, nullptr, nullptr))
      {
        continue;
      }

      HANDLE deviceHandle = CreateFileW(detailData->DevicePath,
                                        GENERIC_READ | GENERIC_WRITE,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        nullptr,
                                        OPEN_EXISTING,
                                        FILE_FLAG_OVERLAPPED,
                                        nullptr);

      if (deviceHandle == INVALID_HANDLE_VALUE)
      {
        deviceHandle = CreateFileW(detailData->DevicePath,
                                   GENERIC_READ,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   nullptr,
                                   OPEN_EXISTING,
                                   FILE_FLAG_OVERLAPPED,
                                   nullptr);
      }

      if (deviceHandle == INVALID_HANDLE_VALUE)
      {
        continue;
      }

      HIDD_ATTRIBUTES attributes;
      ZeroMemory(&attributes, sizeof(attributes));
      attributes.Size = sizeof(attributes);

      if (!HidD_GetAttributes(deviceHandle, &attributes) ||
          attributes.VendorID != 0x054C ||
          (!isDualSenseProductId(attributes.ProductID) && !isDualShock4ProductId(attributes.ProductID)))
      {
        CloseHandle(deviceHandle);
        continue;
      }

      g_dualSenseState.controllerType = isDualShock4ProductId(attributes.ProductID)
                                          ? PSControllerType::DualShock4
                                          : PSControllerType::DualSense;

      PHIDP_PREPARSED_DATA prepData = nullptr;
      if (HidD_GetPreparsedData(deviceHandle, &prepData))
      {
        HIDP_CAPS caps;
        ZeroMemory(&caps, sizeof(caps));
        if (HidP_GetCaps(prepData, &caps) == HIDP_STATUS_SUCCESS)
        {
          const bool isGameControllerCollection = caps.UsagePage == 0x01 &&
                                                 (caps.Usage == 0x04 || caps.Usage == 0x05);
          if (!isGameControllerCollection)
          {
            HidD_FreePreparsedData(prepData);
            CloseHandle(deviceHandle);
            continue;
          }

          if (caps.InputReportByteLength > 0)
          {
          g_dualSenseState.inputReportLength = caps.InputReportByteLength;
          }
        }
        HidD_FreePreparsedData(prepData);
      }

      g_dualSenseState.readEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
      if (g_dualSenseState.readEvent == NULL)
      {
        CloseHandle(deviceHandle);
        continue;
      }

      g_dualSenseState.deviceHandle = deviceHandle;
      g_dualSenseState.transportType = isBluetoothDevicePath(detailData->DevicePath) ? PSTransportType::Bluetooth : PSTransportType::Usb;
      g_dualSenseState.inputBuffer.assign(g_dualSenseState.inputReportLength, 0);
      g_dualSenseState.readPending = false;
      clearDualSenseCachedState();
      if (g_dualSenseState.controllerType == PSControllerType::DualSense)
      {
        requestDualSenseEnhancedReports(deviceHandle);
      }
      found = true;
      break;
    }

    SetupDiDestroyDeviceInfoList(hidInfo);
    return found;
  }

  bool ensureDualSenseConnected()
  {
    if (g_dualSenseState.deviceHandle != INVALID_HANDLE_VALUE)
    {
      return true;
    }

    return discoverDualSense();
  }

  DWORD callXInputGetState(const DWORD userIndex, XINPUT_STATE* state)
  {
    initializeXInput();

    if (!g_xinputGetState)
    {
      if (state)
      {
        ZeroMemory(state, sizeof(XINPUT_STATE));
      }
      return ERROR_DEVICE_NOT_CONNECTED;
    }

    return g_xinputGetState(userIndex, state);
  }

  DWORD callXInputSetState(const DWORD userIndex, XINPUT_VIBRATION* vibration)
  {
    initializeXInput();

    if (!g_xinputSetState)
    {
      return ERROR_DEVICE_NOT_CONNECTED;
    }

    return g_xinputSetState(userIndex, vibration);
  }

  std::string describeXInputBattery(const DWORD userIndex)
  {
    initializeXInput();

    if (!g_xinputGetBatteryInformation)
    {
      return "Battery: Unknown";
    }

    XINPUT_BATTERY_INFORMATION batteryInformation;
    ZeroMemory(&batteryInformation, sizeof(batteryInformation));

    if (g_xinputGetBatteryInformation(userIndex, BATTERY_DEVTYPE_GAMEPAD, &batteryInformation) != ERROR_SUCCESS)
    {
      return "Battery: Unknown";
    }

    if (batteryInformation.BatteryType == BATTERY_TYPE_WIRED)
    {
      return "Battery: Wired";
    }

    if (batteryInformation.BatteryType == BATTERY_TYPE_DISCONNECTED)
    {
      return "Battery: Disconnected";
    }

    switch (batteryInformation.BatteryLevel)
    {
    case BATTERY_LEVEL_EMPTY:
      return "Battery: Empty";
    case BATTERY_LEVEL_LOW:
      return "Battery: Low";
    case BATTERY_LEVEL_MEDIUM:
      return "Battery: Medium";
    case BATTERY_LEVEL_FULL:
      return "Battery: Full";
    default:
      return "Battery: Unknown";
    }
  }

  DWORD getConnectedXInputState(DWORD& userIndex, XINPUT_STATE* state)
  {
    ZeroMemory(state, sizeof(XINPUT_STATE));

    DWORD result = callXInputGetState(userIndex, state);
    if (result == ERROR_SUCCESS)
    {
      return result;
    }

    for (DWORD candidate = 0; candidate < XUSER_MAX_COUNT; ++candidate)
    {
      if (candidate == userIndex)
      {
        continue;
      }

      ZeroMemory(state, sizeof(XINPUT_STATE));
      result = callXInputGetState(candidate, state);
      if (result == ERROR_SUCCESS)
      {
        userIndex = candidate;
        return result;
      }
    }

    ZeroMemory(state, sizeof(XINPUT_STATE));
    return ERROR_DEVICE_NOT_CONNECTED;
  }

  bool tryReadPlayStationInputReport(std::vector<BYTE>& report)
  {
    if (g_dualSenseState.deviceHandle == INVALID_HANDLE_VALUE || g_dualSenseState.readEvent == NULL)
    {
      return false;
    }

    if (g_dualSenseState.inputBuffer.size() != g_dualSenseState.inputReportLength)
    {
      g_dualSenseState.inputBuffer.assign(g_dualSenseState.inputReportLength, 0);
    }

    if (!g_dualSenseState.readPending)
    {
      ZeroMemory(&g_dualSenseState.overlapped, sizeof(g_dualSenseState.overlapped));
      g_dualSenseState.overlapped.hEvent = g_dualSenseState.readEvent;
      ResetEvent(g_dualSenseState.readEvent);

      DWORD bytesRead = 0;
      if (ReadFile(g_dualSenseState.deviceHandle,
                   g_dualSenseState.inputBuffer.data(),
                   static_cast<DWORD>(g_dualSenseState.inputBuffer.size()),
                   &bytesRead,
                   &g_dualSenseState.overlapped))
      {
        report.assign(g_dualSenseState.inputBuffer.begin(), g_dualSenseState.inputBuffer.begin() + bytesRead);
        return bytesRead > 0;
      }

      const DWORD error = GetLastError();
      if (error != ERROR_IO_PENDING)
      {
        closeDualSenseDevice();
        return false;
      }

      g_dualSenseState.readPending = true;
      return false;
    }

    DWORD bytesRead = 0;
    if (!GetOverlappedResult(g_dualSenseState.deviceHandle, &g_dualSenseState.overlapped, &bytesRead, FALSE))
    {
      const DWORD error = GetLastError();
      if (error == ERROR_IO_INCOMPLETE)
      {
        return false;
      }

      closeDualSenseDevice();
      return false;
    }

    g_dualSenseState.readPending = false;
    report.assign(g_dualSenseState.inputBuffer.begin(), g_dualSenseState.inputBuffer.begin() + bytesRead);
    return bytesRead > 0;
  }

  short toThumbAxis(const BYTE value)
  {
    return static_cast<short>((static_cast<int>(value) - 128) * 256);
  }

  void mapDualSenseDpadToXInput(const BYTE dpadNibble, WORD& buttons)
  {
    switch (dpadNibble)
    {
    case 0:
      buttons |= XINPUT_GAMEPAD_DPAD_UP;
      break;
    case 1:
      buttons |= XINPUT_GAMEPAD_DPAD_UP | XINPUT_GAMEPAD_DPAD_RIGHT;
      break;
    case 2:
      buttons |= XINPUT_GAMEPAD_DPAD_RIGHT;
      break;
    case 3:
      buttons |= XINPUT_GAMEPAD_DPAD_RIGHT | XINPUT_GAMEPAD_DPAD_DOWN;
      break;
    case 4:
      buttons |= XINPUT_GAMEPAD_DPAD_DOWN;
      break;
    case 5:
      buttons |= XINPUT_GAMEPAD_DPAD_DOWN | XINPUT_GAMEPAD_DPAD_LEFT;
      break;
    case 6:
      buttons |= XINPUT_GAMEPAD_DPAD_LEFT;
      break;
    case 7:
      buttons |= XINPUT_GAMEPAD_DPAD_LEFT | XINPUT_GAMEPAD_DPAD_UP;
      break;
    default:
      break;
    }
  }

  void mapDualSenseButtons(const BYTE buttons1, const BYTE buttons2, WORD& xButtons)
  {
    mapDualSenseDpadToXInput(static_cast<BYTE>(buttons1 & 0x0F), xButtons);

    if (buttons1 & 0x10) xButtons |= XINPUT_GAMEPAD_X;              // Square
    if (buttons1 & 0x20) xButtons |= XINPUT_GAMEPAD_A;              // Cross
    if (buttons1 & 0x40) xButtons |= XINPUT_GAMEPAD_B;              // Circle
    if (buttons1 & 0x80) xButtons |= XINPUT_GAMEPAD_Y;              // Triangle

    if (buttons2 & 0x01) xButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;  // L1
    if (buttons2 & 0x02) xButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER; // R1
    if (buttons2 & 0x10) xButtons |= XINPUT_GAMEPAD_BACK;           // Create/Share
    if (buttons2 & 0x20) xButtons |= XINPUT_GAMEPAD_START;          // Options
    if (buttons2 & 0x40) xButtons |= XINPUT_GAMEPAD_LEFT_THUMB;     // L3
    if (buttons2 & 0x80) xButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;    // R3
  }

  bool parseDualSenseUsbReport(const std::vector<BYTE>& report, XINPUT_STATE* state)
  {
    if (!state || report.size() < 12 || report[0] != 0x01)
    {
      return false;
    }

    const size_t baseOffset = 1;
    updateDualSenseBatteryStatus(report, baseOffset);
    updateDualSenseTouchpadState(report, baseOffset);

    const BYTE lx = report[baseOffset + 0];
    const BYTE ly = report[baseOffset + 1];
    const BYTE rx = report[baseOffset + 2];
    const BYTE ry = report[baseOffset + 3];
    BYTE l2 = report[baseOffset + 4];
    BYTE r2 = report[baseOffset + 5];
    const BYTE buttons1 = report[baseOffset + 7];
    const BYTE buttons2 = report[baseOffset + 8];

    if (l2 == 0 && (buttons2 & 0x04) != 0)
    {
      l2 = 0xFF;
    }
    if (r2 == 0 && (buttons2 & 0x08) != 0)
    {
      r2 = 0xFF;
    }

    WORD xButtons = 0;
    mapDualSenseButtons(buttons1, buttons2, xButtons);

    state->dwPacketNumber = ++g_dualSenseState.packetNumber;
    state->Gamepad.wButtons = xButtons;
    state->Gamepad.bLeftTrigger = l2;
    state->Gamepad.bRightTrigger = r2;
    state->Gamepad.sThumbLX = toThumbAxis(lx);
    state->Gamepad.sThumbLY = toThumbAxis(static_cast<BYTE>(255 - ly));
    state->Gamepad.sThumbRX = toThumbAxis(rx);
    state->Gamepad.sThumbRY = toThumbAxis(static_cast<BYTE>(255 - ry));

    return true;
  }

  bool parseDualSenseBluetoothSimpleReport(const std::vector<BYTE>& report, XINPUT_STATE* state)
  {
    if (!state || report.size() < 10 || report[0] != 0x01)
    {
      return false;
    }

    resetTouchpadFrame();
    g_playStationBatteryStatus = "Battery: HID status unavailable";

    BYTE l2 = report[8];
    BYTE r2 = report[9];
    const BYTE buttons1 = report[5];
    const BYTE buttons2 = report[6];

    if (l2 == 0 && (buttons2 & 0x04) != 0)
    {
      l2 = 0xFF;
    }
    if (r2 == 0 && (buttons2 & 0x08) != 0)
    {
      r2 = 0xFF;
    }

    WORD xButtons = 0;
    mapDualSenseButtons(buttons1, buttons2, xButtons);

    state->dwPacketNumber = ++g_dualSenseState.packetNumber;
    state->Gamepad.wButtons = xButtons;
    state->Gamepad.bLeftTrigger = l2;
    state->Gamepad.bRightTrigger = r2;
    state->Gamepad.sThumbLX = toThumbAxis(report[1]);
    state->Gamepad.sThumbLY = toThumbAxis(static_cast<BYTE>(255 - report[2]));
    state->Gamepad.sThumbRX = toThumbAxis(report[3]);
    state->Gamepad.sThumbRY = toThumbAxis(static_cast<BYTE>(255 - report[4]));

    return true;
  }

  bool parseDualSenseBluetoothEnhancedReport(const std::vector<BYTE>& report, XINPUT_STATE* state)
  {
    const size_t baseOffset = 2;
    if (!state || report[0] != 0x31 || report.size() <= baseOffset + 52)
    {
      return false;
    }

    updateDualSenseBatteryStatus(report, baseOffset);
    updateDualSenseTouchpadState(report, baseOffset);

    const BYTE lx = report[baseOffset + 0];
    const BYTE ly = report[baseOffset + 1];
    const BYTE rx = report[baseOffset + 2];
    const BYTE ry = report[baseOffset + 3];
    BYTE l2 = report[baseOffset + 4];
    BYTE r2 = report[baseOffset + 5];
    const BYTE buttons1 = report[baseOffset + 7];
    const BYTE buttons2 = report[baseOffset + 8];

    if (l2 == 0 && (buttons2 & 0x04) != 0)
    {
      l2 = 0xFF;
    }
    if (r2 == 0 && (buttons2 & 0x08) != 0)
    {
      r2 = 0xFF;
    }

    WORD xButtons = 0;
    mapDualSenseButtons(buttons1, buttons2, xButtons);

    state->dwPacketNumber = ++g_dualSenseState.packetNumber;
    state->Gamepad.wButtons = xButtons;
    state->Gamepad.bLeftTrigger = l2;
    state->Gamepad.bRightTrigger = r2;
    state->Gamepad.sThumbLX = toThumbAxis(lx);
    state->Gamepad.sThumbLY = toThumbAxis(static_cast<BYTE>(255 - ly));
    state->Gamepad.sThumbRX = toThumbAxis(rx);
    state->Gamepad.sThumbRY = toThumbAxis(static_cast<BYTE>(255 - ry));

    return true;
  }

  bool parseDualSenseReport(const std::vector<BYTE>& report, XINPUT_STATE* state)
  {
    if (report.empty())
    {
      return false;
    }

    if (report[0] == 0x31)
    {
      return parseDualSenseBluetoothEnhancedReport(report, state);
    }

    if (report[0] != 0x01)
    {
      return false;
    }

    const bool isBluetoothSimple = g_dualSenseState.transportType == PSTransportType::Bluetooth || report.size() <= 10;
    if (isBluetoothSimple)
    {
      return parseDualSenseBluetoothSimpleReport(report, state);
    }

    return parseDualSenseUsbReport(report, state);
  }

  // DualShock 4 (PS4) USB report: Report ID 0x01, data starts at byte 1.
  // Layout: LX LY RX RY buttons1 buttons2 PS/TP L2 R2 ...
  // Bluetooth report: Report ID 0x11, data starts at byte 3.
  bool parseDualShock4Report(const std::vector<BYTE>& report, XINPUT_STATE* state)
  {
    if (!state || report.size() < 10)
    {
      return false;
    }

    size_t baseOffset = 0;
    if (report[0] == 0x01)
    {
      baseOffset = 1; // USB
    }
    else if (report[0] == 0x11)
    {
      baseOffset = 3; // Bluetooth
    }
    else
    {
      return false;
    }

    if (report.size() <= baseOffset + 8)
    {
      return false;
    }

    updateDualShock4BatteryStatus(report, baseOffset);
    resetTouchpadFrame();

    const BYTE lx       = report[baseOffset + 0];
    const BYTE ly       = report[baseOffset + 1];
    const BYTE rx       = report[baseOffset + 2];
    const BYTE ry       = report[baseOffset + 3];
    const BYTE buttons1 = report[baseOffset + 4]; // dpad nibble (low) + Square/Cross/Circle/Triangle (high)
    const BYTE buttons2 = report[baseOffset + 5]; // L1/R1/L2/R2/Share/Options/L3/R3
    const BYTE l2       = report[baseOffset + 7];
    const BYTE r2       = report[baseOffset + 8];

    WORD xButtons = 0;

    mapDualSenseDpadToXInput(static_cast<BYTE>(buttons1 & 0x0F), xButtons);

    if (buttons1 & 0x10) xButtons |= XINPUT_GAMEPAD_X;              // Square
    if (buttons1 & 0x20) xButtons |= XINPUT_GAMEPAD_A;              // Cross
    if (buttons1 & 0x40) xButtons |= XINPUT_GAMEPAD_B;              // Circle
    if (buttons1 & 0x80) xButtons |= XINPUT_GAMEPAD_Y;              // Triangle

    if (buttons2 & 0x01) xButtons |= XINPUT_GAMEPAD_LEFT_SHOULDER;  // L1
    if (buttons2 & 0x02) xButtons |= XINPUT_GAMEPAD_RIGHT_SHOULDER; // R1
    if (buttons2 & 0x10) xButtons |= XINPUT_GAMEPAD_BACK;           // Share
    if (buttons2 & 0x20) xButtons |= XINPUT_GAMEPAD_START;          // Options
    if (buttons2 & 0x40) xButtons |= XINPUT_GAMEPAD_LEFT_THUMB;     // L3
    if (buttons2 & 0x80) xButtons |= XINPUT_GAMEPAD_RIGHT_THUMB;    // R3

    state->dwPacketNumber = ++g_dualSenseState.packetNumber;
    state->Gamepad.wButtons = xButtons;
    state->Gamepad.bLeftTrigger = l2;
    state->Gamepad.bRightTrigger = r2;
    state->Gamepad.sThumbLX = toThumbAxis(lx);
    state->Gamepad.sThumbLY = toThumbAxis(static_cast<BYTE>(255 - ly));
    state->Gamepad.sThumbRX = toThumbAxis(rx);
    state->Gamepad.sThumbRY = toThumbAxis(static_cast<BYTE>(255 - ry));

    return true;
  }

  bool readDualSenseState(const DWORD userIndex, XINPUT_STATE* state)
  {
    if (userIndex != 0 || !state)
    {
      return false;
    }

    if (!ensureDualSenseConnected())
    {
      resetTouchpadFrame();
      return false;
    }

    const bool isDualShock4 = (g_dualSenseState.controllerType == PSControllerType::DualShock4);

    std::vector<BYTE> report;
    const bool hasFreshReport = tryReadPlayStationInputReport(report);
    if (hasFreshReport)
    {
      XINPUT_STATE parsedState;
      ZeroMemory(&parsedState, sizeof(parsedState));

      const bool parsed = isDualShock4 ? parseDualShock4Report(report, &parsedState)
                                       : parseDualSenseReport(report, &parsedState);
      if (!parsed)
      {
        closeDualSenseDevice();
        return false;
      }

      g_dualSenseState.cachedState = parsedState;
      g_dualSenseState.hasCachedState = true;
      g_dualSenseState.lastValidReportTick = GetTickCount();
    }
    else if (!isDualShock4)
    {
      clearTouchpadDeltas();

      if (g_dualSenseState.deviceHandle == INVALID_HANDLE_VALUE)
      {
        clearDualSenseCachedState();
        return false;
      }

      const DWORD STALE_REPORT_TIMEOUT_MS = 250;
      if (g_dualSenseState.hasCachedState &&
          g_dualSenseState.lastValidReportTick != 0 &&
          (GetTickCount() - g_dualSenseState.lastValidReportTick) > STALE_REPORT_TIMEOUT_MS)
      {
        ZeroMemory(&g_dualSenseState.cachedState, sizeof(g_dualSenseState.cachedState));
        resetTouchpadFrame();
      }
    }

    if (g_dualSenseState.hasCachedState)
    {
      *state = g_dualSenseState.cachedState;
      return true;
    }

    ZeroMemory(state, sizeof(XINPUT_STATE));
    return true;
  }
}

CXBOXController::CXBOXController(int playerNumber)
{
  _controllerNum = playerNumber - 1; //set number
}

XINPUT_STATE CXBOXController::GetState()
{
  DWORD controllerIndex = static_cast<DWORD>(_controllerNum);
  if (getConnectedXInputState(controllerIndex, &this->_controllerState) == ERROR_SUCCESS)
  {
    _controllerNum = static_cast<int>(controllerIndex);
    _isConnected = true;
    _controllerType = "XInput Controller";
    _batteryStatus = describeXInputBattery(controllerIndex);
    resetTouchpadFrame();
    return _controllerState;
  }

  if (readDualSenseState(_controllerNum, &this->_controllerState))
  {
    _isConnected = true;
    _controllerType = g_dualSenseState.controllerType == PSControllerType::DualShock4
      ? "DualShock 4 (HID)"
      : "DualSense (HID)";
    _batteryStatus = g_playStationBatteryStatus;
  }
  else
  {
    _isConnected = false;
    _controllerType = "Disconnected";
    _batteryStatus = "Battery: Disconnected";
    ZeroMemory(&this->_controllerState, sizeof(this->_controllerState));
    resetTouchpadFrame();
  }

  return _controllerState;
}

bool CXBOXController::IsConnected()
{
  DWORD controllerIndex = static_cast<DWORD>(_controllerNum);
  if (getConnectedXInputState(controllerIndex, &this->_controllerState) == ERROR_SUCCESS)
  {
    _controllerNum = static_cast<int>(controllerIndex);
    _isConnected = true;
    _controllerType = "XInput Controller";
    _batteryStatus = describeXInputBattery(controllerIndex);
    resetTouchpadFrame();
    return true;
  }

  _isConnected = readDualSenseState(_controllerNum, &this->_controllerState);
  _controllerType = _isConnected
    ? (g_dualSenseState.controllerType == PSControllerType::DualShock4 ? "DualShock 4 (HID)" : "DualSense (HID)")
    : "Disconnected";
  _batteryStatus = _isConnected ? g_playStationBatteryStatus : "Battery: Disconnected";
  if (!_isConnected)
  {
    ZeroMemory(&this->_controllerState, sizeof(this->_controllerState));
    resetTouchpadFrame();
  }
  return _isConnected;
}

void CXBOXController::Vibrate(int leftVal, int rightVal)
{
  // Create a Vibraton State
  XINPUT_VIBRATION Vibration;

  // Zeroise the Vibration
  ZeroMemory(&Vibration, sizeof(XINPUT_VIBRATION));

  // Set the Vibration Values
  Vibration.wLeftMotorSpeed = leftVal;
  Vibration.wRightMotorSpeed = rightVal;

  // Vibrate the controller
  callXInputSetState(_controllerNum, &Vibration);
}

bool CXBOXController::GetLastConnectionState() const
{
  return _isConnected;
}

const std::string& CXBOXController::GetControllerTypeName() const
{
  return _controllerType;
}

const std::string& CXBOXController::GetBatteryStatus() const
{
  return _batteryStatus;
}

CXBOXController::TouchpadState CXBOXController::GetTouchpadState() const
{
  TouchpadState state = {};
  state.available = g_dualSenseState.controllerType == PSControllerType::DualSense && g_dualSenseState.touchpad.available;
  state.active = state.available && g_dualSenseState.touchpad.active;
  state.deltaX = state.available ? g_dualSenseState.touchpad.deltaX : 0;
  state.deltaY = state.available ? g_dualSenseState.touchpad.deltaY : 0;
  return state;
}