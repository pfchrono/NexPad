#pragma once

#include <string>

#include <windows.h>
#include <xinput.h>

class CXBOXController
{
public:
  enum class BatteryLevelCategory
  {
    Unknown,
    Empty,
    Low,
    Medium,
    High,
  };

  struct TouchpadState
  {
    bool active = false;
    bool available = false;
    bool reliableTwoFinger = false;
    unsigned char activeFingerCount = 0;
    short deltaX = 0;
    short deltaY = 0;
    short scrollDeltaX = 0;
    short scrollDeltaY = 0;
  };

  struct BatteryPresentationState
  {
    bool connected = false;
    bool available = false;
    bool charging = false;
    bool wired = false;
    BatteryLevelCategory level = BatteryLevelCategory::Unknown;
    unsigned char filledSegments = 0;
    unsigned char totalSegments = 4;
    std::string detailText = "Battery: Unknown";
  };

private:
  XINPUT_STATE _controllerState;
  int _controllerNum;
  bool _isConnected = false;
  std::string _controllerType = "Disconnected";
  std::string _batteryStatus = "Battery: Unknown";
  BatteryPresentationState _batteryPresentationState = {};

public:
  CXBOXController(int playerNumber);
  XINPUT_STATE GetState();
  bool IsConnected();
  void Vibrate(int leftVal, int rightVal);
  bool GetLastConnectionState() const;
  const std::string &GetControllerTypeName() const;
  const std::string &GetBatteryStatus() const;
  BatteryPresentationState GetBatteryPresentationState() const;
  TouchpadState GetTouchpadState() const;
};
