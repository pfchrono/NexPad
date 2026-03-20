#pragma once

#include <string>

#include <windows.h>
#include <xinput.h>

class CXBOXController
{
public:
  struct TouchpadState
  {
    bool active = false;
    bool available = false;
    short deltaX = 0;
    short deltaY = 0;
  };

private:
  XINPUT_STATE _controllerState;
  int _controllerNum;
  bool _isConnected = false;
  std::string _controllerType = "Disconnected";
  std::string _batteryStatus = "Battery: Unknown";
public:
  CXBOXController(int playerNumber);
  XINPUT_STATE GetState();
  bool IsConnected();
  void Vibrate(int leftVal, int rightVal);
  bool GetLastConnectionState() const;
  const std::string& GetControllerTypeName() const;
  const std::string& GetBatteryStatus() const;
  TouchpadState GetTouchpadState() const;
};
