#include "NexPad.h"
#include "ConfigFile.h"

#include <fstream>
#include <iomanip>

namespace
{
  bool hasTickElapsed(const DWORD now, const DWORD target)
  {
    return static_cast<LONG>(now - target) >= 0;
  }

  const char *kStartupRunKeyPath = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
  const char *kStartupRunValueName = "NexPad";

  std::string getExecutableDirectory()
  {
    char path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
    {
      return ".";
    }

    std::string fullPath(path, length);
    const size_t separator = fullPath.find_last_of("\\/");
    if (separator == std::string::npos)
    {
      return ".";
    }

    return fullPath.substr(0, separator);
  }

  std::string getExecutablePath()
  {
    char path[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
    {
      return std::string();
    }

    return std::string(path, length);
  }

  std::string joinPath(const std::string &left, const std::string &right)
  {
    if (left.empty() || left == ".")
    {
      return right;
    }

    if (left.back() == '\\' || left.back() == '/')
    {
      return left + right;
    }

    return left + "\\" + right;
  }

  void appendHexSetting(std::ostringstream &stream, const std::string &key, DWORD value)
  {
    stream << key << " = 0x"
           << std::uppercase << std::hex << value
           << std::nouppercase << std::dec
           << "\r\n";
  }

  bool tryParseConfigValue(const std::string &text, const std::string &key, DWORD &value)
  {
    std::istringstream input(text);
    for (std::string line; std::getline(input, line);)
    {
      const size_t commentPos = line.find('#');
      if (commentPos != std::string::npos)
      {
        line.erase(commentPos);
      }

      const size_t firstNonWhitespace = line.find_first_not_of("\t ");
      if (firstNonWhitespace == std::string::npos)
      {
        continue;
      }

      line.erase(0, firstNonWhitespace);
      const size_t separatorPos = line.find('=');
      if (separatorPos == std::string::npos)
      {
        continue;
      }

      std::string lineKey = line.substr(0, separatorPos);
      lineKey.erase(lineKey.find_last_not_of("\t ") + 1);
      if (lineKey != key)
      {
        continue;
      }

      std::string valueText = line.substr(separatorPos + 1);
      const size_t valueStart = valueText.find_first_not_of("\t ");
      if (valueStart == std::string::npos)
      {
        value = 0;
        return true;
      }

      valueText.erase(0, valueStart);
      const size_t valueEnd = valueText.find_last_not_of("\t ");
      if (valueEnd != std::string::npos)
      {
        valueText.erase(valueEnd + 1);
      }

      value = static_cast<DWORD>(strtoul(valueText.c_str(), 0, 0));
      return true;
    }

    return false;
  }

  bool tryParseFloatConfigValue(const std::string &text, const std::string &key, float &value)
  {
    std::istringstream input(text);
    for (std::string line; std::getline(input, line);)
    {
      const size_t commentPos = line.find('#');
      if (commentPos != std::string::npos)
      {
        line.erase(commentPos);
      }

      const size_t firstNonWhitespace = line.find_first_not_of("\t ");
      if (firstNonWhitespace == std::string::npos)
      {
        continue;
      }

      line.erase(0, firstNonWhitespace);
      const size_t separatorPos = line.find('=');
      if (separatorPos == std::string::npos)
      {
        continue;
      }

      std::string lineKey = line.substr(0, separatorPos);
      lineKey.erase(lineKey.find_last_not_of("\t ") + 1);
      if (lineKey != key)
      {
        continue;
      }

      std::string valueText = line.substr(separatorPos + 1);
      const size_t valueStart = valueText.find_first_not_of("\t ");
      if (valueStart == std::string::npos)
      {
        return false;
      }

      valueText.erase(0, valueStart);
      const size_t valueEnd = valueText.find_last_not_of("\t ");
      if (valueEnd != std::string::npos)
      {
        valueText.erase(valueEnd + 1);
      }

      value = static_cast<float>(atof(valueText.c_str()));
      return true;
    }

    return false;
  }

  std::string trimWhitespace(const std::string &text)
  {
    const size_t start = text.find_first_not_of("\t \r\n");
    if (start == std::string::npos)
    {
      return std::string();
    }

    const size_t end = text.find_last_not_of("\t \r\n");
    return text.substr(start, end - start + 1);
  }

  std::string extractExecutablePathFromCommand(const std::string &command)
  {
    const std::string trimmed = trimWhitespace(command);
    if (trimmed.empty())
    {
      return std::string();
    }

    if (trimmed[0] == '"')
    {
      const size_t closingQuote = trimmed.find('"', 1);
      if (closingQuote != std::string::npos)
      {
        return trimmed.substr(1, closingQuote - 1);
      }
    }

    const size_t firstWhitespace = trimmed.find_first_of("\t ");
    if (firstWhitespace == std::string::npos)
    {
      return trimmed;
    }

    return trimmed.substr(0, firstWhitespace);
  }

  std::string formatWindowsErrorMessage(const LONG errorCode)
  {
    LPSTR buffer = NULL;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD length = FormatMessageA(flags, NULL, static_cast<DWORD>(errorCode), 0, reinterpret_cast<LPSTR>(&buffer), 0, NULL);
    if (length == 0 || buffer == NULL)
    {
      std::ostringstream fallback;
      fallback << "Windows error " << errorCode;
      return fallback.str();
    }

    std::string message(buffer, length);
    LocalFree(buffer);
    return trimWhitespace(message);
  }

  bool queryStartWithWindowsRegistration(std::string &registeredCommand, std::string *errorMessage)
  {
    registeredCommand.clear();

    HKEY runKey = NULL;
    const LONG openStatus = RegOpenKeyExA(HKEY_CURRENT_USER, kStartupRunKeyPath, 0, KEY_QUERY_VALUE, &runKey);
    if (openStatus == ERROR_FILE_NOT_FOUND)
    {
      return false;
    }

    if (openStatus != ERROR_SUCCESS)
    {
      if (errorMessage != NULL)
      {
        *errorMessage = formatWindowsErrorMessage(openStatus);
      }
      return false;
    }

    char valueBuffer[2048] = {};
    DWORD valueType = 0;
    DWORD valueSize = sizeof(valueBuffer);
    const LONG queryStatus = RegQueryValueExA(runKey, kStartupRunValueName, NULL, &valueType, reinterpret_cast<LPBYTE>(valueBuffer), &valueSize);
    RegCloseKey(runKey);

    if (queryStatus == ERROR_FILE_NOT_FOUND)
    {
      return false;
    }

    if (queryStatus != ERROR_SUCCESS)
    {
      if (errorMessage != NULL)
      {
        *errorMessage = formatWindowsErrorMessage(queryStatus);
      }
      return false;
    }

    if (valueType != REG_SZ && valueType != REG_EXPAND_SZ)
    {
      if (errorMessage != NULL)
      {
        *errorMessage = "Startup registration exists but is not a string value.";
      }
      return false;
    }

    registeredCommand = trimWhitespace(valueBuffer);
    return !registeredCommand.empty();
  }

  bool isCurrentExecutableRegisteredForStartup(std::string *registeredCommand, std::string *errorMessage)
  {
    std::string startupCommand;
    if (!queryStartWithWindowsRegistration(startupCommand, errorMessage))
    {
      if (registeredCommand != NULL)
      {
        registeredCommand->clear();
      }
      return false;
    }

    if (registeredCommand != NULL)
    {
      *registeredCommand = startupCommand;
    }

    const std::string registeredPath = extractExecutablePathFromCommand(startupCommand);
    const std::string executablePath = getExecutablePath();
    if (registeredPath.empty() || executablePath.empty())
    {
      return false;
    }

    return _stricmp(registeredPath.c_str(), executablePath.c_str()) == 0;
  }

  bool writeStartWithWindowsRegistration(const bool enabled, std::string *errorMessage)
  {
    HKEY runKey = NULL;
    DWORD disposition = 0;
    const LONG createStatus = RegCreateKeyExA(HKEY_CURRENT_USER, kStartupRunKeyPath, 0, NULL, 0, KEY_QUERY_VALUE | KEY_SET_VALUE, NULL, &runKey, &disposition);
    if (createStatus != ERROR_SUCCESS)
    {
      if (errorMessage != NULL)
      {
        *errorMessage = formatWindowsErrorMessage(createStatus);
      }
      return false;
    }

    LONG writeStatus = ERROR_SUCCESS;
    if (enabled)
    {
      const std::string executablePath = getExecutablePath();
      if (executablePath.empty())
      {
        RegCloseKey(runKey);
        if (errorMessage != NULL)
        {
          *errorMessage = "Unable to resolve the current NexPad executable path.";
        }
        return false;
      }

      const std::string command = "\"" + executablePath + "\"";
      writeStatus = RegSetValueExA(runKey, kStartupRunValueName, 0, REG_SZ, reinterpret_cast<const BYTE *>(command.c_str()), static_cast<DWORD>(command.size() + 1));
    }
    else
    {
      writeStatus = RegDeleteValueA(runKey, kStartupRunValueName);
      if (writeStatus == ERROR_FILE_NOT_FOUND)
      {
        writeStatus = ERROR_SUCCESS;
      }
    }

    RegCloseKey(runKey);

    if (writeStatus != ERROR_SUCCESS)
    {
      if (errorMessage != NULL)
      {
        *errorMessage = formatWindowsErrorMessage(writeStatus);
      }
      return false;
    }

    return true;
  }
}

// Description:
//   Send a keyboard input to the system based on the key value
//     and its event type.
//
// Params:
//   cmd    The value of the key to send(see http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx)
//   flag   The KEYEVENT for the key
void inputKeyboard(WORD cmd, DWORD flag)
{
  INPUT input;
  input.type = INPUT_KEYBOARD;
  input.ki.wScan = 0;
  input.ki.time = 0;
  input.ki.dwExtraInfo = 0;
  input.ki.wVk = cmd;
  input.ki.dwFlags = flag;
  SendInput(1, &input, sizeof(INPUT));
}

// Description:
//   Send a keyboard input based on the key value with the "pressed down" event.
//
// Params:
//   cmd    The value of the key to send
void inputKeyboardDown(WORD cmd)
{
  inputKeyboard(cmd, 0);
}

// Description:
//   Send a keyboard input based on the key value with the "released" event
//
// Params:
//   cmd    The value of the key to send
void inputKeyboardUp(WORD cmd)
{
  inputKeyboard(cmd, KEYEVENTF_KEYUP);
}

// Description:
//   Send a mouse input based on a mouse event type.
//   See https://msdn.microsoft.com/en-us/library/windows/desktop/ms646310(v=vs.85).aspx
//
// Params:
//   dwFlags    The mouse event to send
//   mouseData  Additional information needed for certain mouse events (Optional)
void mouseEvent(DWORD dwFlags, DWORD mouseData = 0)
{
  INPUT input;
  input.type = INPUT_MOUSE;

  // Only set mouseData when using a supported dwFlags type
  if (dwFlags == MOUSEEVENTF_WHEEL ||
      dwFlags == MOUSEEVENTF_XUP ||
      dwFlags == MOUSEEVENTF_XDOWN ||
      dwFlags == MOUSEEVENTF_HWHEEL)
  {
    input.mi.mouseData = mouseData;
  }
  else
  {
    input.mi.mouseData = 0;
  }

  input.mi.dwFlags = dwFlags;
  input.mi.time = 0;
  SendInput(1, &input, sizeof(INPUT));
}

NexPad::NexPad(CXBOXController *controller)
    : _controller(controller)
{
  _configPath = joinPath(getExecutableDirectory(), "config.ini");
}

void NexPad::setWindowHandle(HWND windowHandle)
{
  _windowHandle = windowHandle;
}

void NexPad::setStatusCallback(const std::function<void(const std::string &)> &callback)
{
  _statusCallback = callback;
}

void NexPad::setCurrentState(const XINPUT_STATE &state)
{
  _currentState = state;
}

bool NexPad::isDisabled() const
{
  return _disabled;
}

bool NexPad::isVibrationDisabled() const
{
  return _vibrationDisabled;
}

bool NexPad::isWindowHidden() const
{
  return _hidden;
}

float NexPad::getCurrentSpeed() const
{
  return speed;
}

float NexPad::getScrollSpeed() const
{
  return SCROLL_SPEED;
}

int NexPad::getTouchpadEnabled() const
{
  return TOUCHPAD_ENABLED;
}

int NexPad::getTouchpadDeadZone() const
{
  return TOUCHPAD_DEAD_ZONE;
}

float NexPad::getTouchpadSpeed() const
{
  return TOUCHPAD_SPEED;
}

int NexPad::getLoopIntervalMs() const
{
  return SLEEP_AMOUNT;
}

const std::vector<float> &NexPad::getSpeeds() const
{
  return speeds;
}

const std::vector<std::string> &NexPad::getSpeedNames() const
{
  return speed_names;
}

unsigned int NexPad::getSpeedIndex() const
{
  return speed_idx;
}

int NexPad::getSwapThumbsticks() const
{
  return SWAP_THUMBSTICKS;
}

int NexPad::getStartWithWindows() const
{
  return START_WITH_WINDOWS;
}

int NexPad::getUiThemeMode() const
{
  return UI_THEME_MODE;
}

const std::string &NexPad::getConfigPath() const
{
  return _configPath;
}

void NexPad::setSpeedIndex(unsigned int index)
{
  if (index < speeds.size())
  {
    speed_idx = index;
    speed = speeds[speed_idx];

    std::ostringstream speedMessage;
    speedMessage << "Applied speed " << speed << " (" << speed_names[speed_idx] << ")";
    notifyStatus(speedMessage.str());
  }
}

void NexPad::setScrollSpeed(float value)
{
  if (value > 0.0f)
  {
    SCROLL_SPEED = value;

    std::ostringstream scrollMessage;
    scrollMessage << "Applied scroll speed " << SCROLL_SPEED;
    notifyStatus(scrollMessage.str());
  }
}

void NexPad::setTouchpadEnabled(int value)
{
  TOUCHPAD_ENABLED = value ? 1 : 0;
  notifyStatus(std::string("DualSense touchpad cursor ") + (TOUCHPAD_ENABLED ? "Enabled" : "Disabled"));
}

void NexPad::setTouchpadDeadZone(int value)
{
  if (value >= 0)
  {
    TOUCHPAD_DEAD_ZONE = value;

    std::ostringstream touchpadDeadZoneMessage;
    touchpadDeadZoneMessage << "Applied touchpad dead zone " << TOUCHPAD_DEAD_ZONE;
    notifyStatus(touchpadDeadZoneMessage.str());
  }
}

void NexPad::setTouchpadSpeed(float value)
{
  if (value > 0.0f)
  {
    TOUCHPAD_SPEED = value;

    std::ostringstream touchpadMessage;
    touchpadMessage << "Applied touchpad speed " << TOUCHPAD_SPEED;
    notifyStatus(touchpadMessage.str());
  }
}

void NexPad::setSwapThumbsticks(int value)
{
  SWAP_THUMBSTICKS = value ? 1 : 0;
  notifyStatus(std::string("Swap thumbsticks ") + (SWAP_THUMBSTICKS ? "Enabled" : "Disabled"));
}

bool NexPad::applyStartWithWindowsSetting(bool enabled, bool notify, std::string *errorMessage)
{
  if (errorMessage != NULL)
  {
    errorMessage->clear();
  }

  std::string registryError;
  if (!writeStartWithWindowsRegistration(enabled, &registryError))
  {
    std::string currentCommand;
    START_WITH_WINDOWS = isCurrentExecutableRegisteredForStartup(&currentCommand, NULL) ? 1 : 0;
    if (errorMessage != NULL)
    {
      *errorMessage = registryError;
    }
    return false;
  }

  std::string currentCommand;
  START_WITH_WINDOWS = isCurrentExecutableRegisteredForStartup(&currentCommand, NULL) ? 1 : 0;
  if (enabled && START_WITH_WINDOWS == 0)
  {
    if (errorMessage != NULL)
    {
      *errorMessage = "Startup registration did not match the current NexPad executable path after saving.";
    }
    return false;
  }

  if (notify)
  {
    notifyStatus(std::string("Start with Windows ") + (START_WITH_WINDOWS ? "Enabled" : "Disabled"));
  }

  return true;
}

bool NexPad::setStartWithWindows(int value, std::string &errorMessage)
{
  return applyStartWithWindowsSetting(value != 0, true, &errorMessage);
}

void NexPad::setUiThemeMode(int value)
{
  if (value < 0)
  {
    value = 0;
  }
  else if (value > 2)
  {
    value = 2;
  }

  UI_THEME_MODE = value;

  if (UI_THEME_MODE == 1)
  {
    notifyStatus("UI theme set to Light");
  }
  else if (UI_THEME_MODE == 2)
  {
    notifyStatus("UI theme set to High Contrast");
  }
  else
  {
    notifyStatus("UI theme set to Dark");
  }
}

void NexPad::notifyStatus(const std::string &message) const
{
  if (_statusCallback)
  {
    _statusCallback(message);
  }
}

// Description:
//   Reads and parses the configuration file, assigning values to the
//     configuration variables.
void NexPad::loadConfigFile()
{
  ConfigFile cfg(_configPath);
  speeds.clear();
  speed_names.clear();

  //--------------------------------
  // Configuration bindings
  //--------------------------------
  CONFIG_MOUSE_LEFT = strtol(cfg.getValueOfKey<std::string>("CONFIG_MOUSE_LEFT").c_str(), 0, 0);
  CONFIG_MOUSE_RIGHT = strtol(cfg.getValueOfKey<std::string>("CONFIG_MOUSE_RIGHT").c_str(), 0, 0);
  CONFIG_MOUSE_MIDDLE = strtol(cfg.getValueOfKey<std::string>("CONFIG_MOUSE_MIDDLE").c_str(), 0, 0);
  CONFIG_HIDE = strtol(cfg.getValueOfKey<std::string>("CONFIG_HIDE").c_str(), 0, 0);
  CONFIG_DISABLE = strtol(cfg.getValueOfKey<std::string>("CONFIG_DISABLE").c_str(), 0, 0);
  CONFIG_DISABLE_VIBRATION = strtol(cfg.getValueOfKey<std::string>("CONFIG_DISABLE_VIBRATION").c_str(), 0, 0);
  CONFIG_SPEED_CHANGE = strtol(cfg.getValueOfKey<std::string>("CONFIG_SPEED_CHANGE").c_str(), 0, 0);
  CONFIG_OSK = strtol(cfg.getValueOfKey<std::string>("CONFIG_OSK").c_str(), 0, 0);

  //--------------------------------
  // Controller bindings
  //--------------------------------
  GAMEPAD_DPAD_UP = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_DPAD_UP").c_str(), 0, 0);
  GAMEPAD_DPAD_DOWN = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_DPAD_DOWN").c_str(), 0, 0);
  GAMEPAD_DPAD_LEFT = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_DPAD_LEFT").c_str(), 0, 0);
  GAMEPAD_DPAD_RIGHT = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_DPAD_RIGHT").c_str(), 0, 0);
  GAMEPAD_START = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_START").c_str(), 0, 0);
  GAMEPAD_BACK = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_BACK").c_str(), 0, 0);
  GAMEPAD_LEFT_THUMB = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_LEFT_THUMB").c_str(), 0, 0);
  GAMEPAD_RIGHT_THUMB = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_RIGHT_THUMB").c_str(), 0, 0);
  GAMEPAD_LEFT_SHOULDER = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_LEFT_SHOULDER").c_str(), 0, 0);
  GAMEPAD_RIGHT_SHOULDER = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_RIGHT_SHOULDER").c_str(), 0, 0);
  GAMEPAD_A = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_A").c_str(), 0, 0);
  GAMEPAD_B = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_B").c_str(), 0, 0);
  GAMEPAD_X = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_X").c_str(), 0, 0);
  GAMEPAD_Y = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_Y").c_str(), 0, 0);
  GAMEPAD_TRIGGER_LEFT = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_TRIGGER_LEFT").c_str(), 0, 0);
  GAMEPAD_TRIGGER_RIGHT = strtol(cfg.getValueOfKey<std::string>("GAMEPAD_TRIGGER_RIGHT").c_str(), 0, 0);
  ON_ENABLE = strtol(cfg.getValueOfKey<std::string>("ON_ENABLE").c_str(), 0, 0);
  ON_DISABLE = strtol(cfg.getValueOfKey<std::string>("ON_DISABLE").c_str(), 0, 0);
  //--------------------------------
  // Advanced settings
  //--------------------------------

  INIT_DISABLED = std::stoi(cfg.getValueOfKey<std::string>("INIT_DISABLED").c_str());
  _disabled = INIT_DISABLED;

  // Acceleration factor
  acceleration_factor = strtof(cfg.getValueOfKey<std::string>("ACCELERATION_FACTOR").c_str(), 0);

  // Dead zones
  DEAD_ZONE = strtol(cfg.getValueOfKey<std::string>("DEAD_ZONE").c_str(), 0, 0);
  if (DEAD_ZONE == 0)
  {
    DEAD_ZONE = 6000;
  }

  SCROLL_DEAD_ZONE = strtol(cfg.getValueOfKey<std::string>("SCROLL_DEAD_ZONE").c_str(), 0, 0);
  if (SCROLL_DEAD_ZONE == 0)
  {
    SCROLL_DEAD_ZONE = 5000;
  }

  SCROLL_SPEED = strtof(cfg.getValueOfKey<std::string>("SCROLL_SPEED").c_str(), 0);
  if (SCROLL_SPEED < 0.00001f)
  {
    SCROLL_SPEED = 0.1f;
  }

  TOUCHPAD_ENABLED = strtol(cfg.getValueOfKey<std::string>("TOUCHPAD_ENABLED", "1").c_str(), 0, 0) != 0 ? 1 : 0;

  TOUCHPAD_DEAD_ZONE = strtol(cfg.getValueOfKey<std::string>("TOUCHPAD_DEAD_ZONE", "2").c_str(), 0, 0);
  if (TOUCHPAD_DEAD_ZONE < 0)
  {
    TOUCHPAD_DEAD_ZONE = 0;
  }

  TOUCHPAD_SPEED = strtof(cfg.getValueOfKey<std::string>("TOUCHPAD_SPEED", "1.200").c_str(), 0);
  if (TOUCHPAD_SPEED < 0.00001f)
  {
    TOUCHPAD_SPEED = 1.2f;
  }

  START_WITH_WINDOWS = strtol(cfg.getValueOfKey<std::string>("START_WITH_WINDOWS", "0").c_str(), 0, 0) != 0 ? 1 : 0;
  std::string startupError;
  if (!applyStartWithWindowsSetting(START_WITH_WINDOWS != 0, false, &startupError))
  {
    notifyStatus("Unable to sync Start with Windows setting: " + startupError);
  }

  UI_THEME_MODE = static_cast<int>(strtol(cfg.getValueOfKey<std::string>("UI_THEME_MODE", "0").c_str(), 0, 0));
  if (UI_THEME_MODE < 0)
  {
    UI_THEME_MODE = 0;
  }
  else if (UI_THEME_MODE > 2)
  {
    UI_THEME_MODE = 2;
  }

  unsigned int configuredSpeedIndex = static_cast<unsigned int>(strtoul(cfg.getValueOfKey<std::string>("CURRENT_SPEED_INDEX", "0").c_str(), 0, 0));

  // Variable cursor speeds
  std::istringstream cursor_speed = std::istringstream(cfg.getValueOfKey<std::string>("CURSOR_SPEED"));
  int cur_speed_idx = 1;
  const float CUR_SPEED_MIN = 0.0001f;
  const float CUR_SPEED_MAX = 1.0f;
  for (std::string cur_speed; std::getline(cursor_speed, cur_speed, ',');)
  {
    std::istringstream cursor_speed_entry = std::istringstream(cur_speed);
    std::string cur_name, cur_speed_s;
    // Check to see if we are at the string that includes the equals sign.
    if (cur_speed.find_first_of('=') != std::string::npos)
    {
      std::getline(cursor_speed_entry, cur_name, '=');
    }
    else
    {
      std::ostringstream tmp_name;
      tmp_name << cur_speed_idx++;
      cur_name = tmp_name.str();
    }
    std::getline(cursor_speed_entry, cur_speed_s);
    float cur_speedf = strtof(cur_speed_s.c_str(), 0);
    // Ignore speeds that are not within the allowed range.
    if (cur_speedf > CUR_SPEED_MIN && cur_speedf <= CUR_SPEED_MAX)
    {
      speeds.push_back(cur_speedf);
      speed_names.push_back(cur_name);
    }
  }

  // If no cursor speeds were defined, add a set of default speeds.
  if (speeds.size() == 0)
  {
    speeds.push_back(0.005f);
    speeds.push_back(0.015f);
    speeds.push_back(0.025f);
    speeds.push_back(0.004f);
    speed_names.push_back("ULTRALOW");
    speed_names.push_back("LOW");
    speed_names.push_back("MED");
    speed_names.push_back("HIGH");
  }
  if (configuredSpeedIndex >= speeds.size())
  {
    configuredSpeedIndex = 0;
  }
  speed_idx = configuredSpeedIndex;
  speed = speeds[speed_idx];

  // Swap stick functions
  SWAP_THUMBSTICKS = strtol(cfg.getValueOfKey<std::string>("SWAP_THUMBSTICKS").c_str(), 0, 0);

  // Set the initial window visibility
  setWindowVisibility(_hidden);
  notifyStatus("Loaded configuration from " + _configPath);
}

namespace
{
  bool updateConfigLine(std::vector<std::string> &lines, const std::string &key, const std::string &value)
  {
    const std::string prefix = key + " =";
    for (std::string &line : lines)
    {
      std::string trimmed = line;
      trimmed.erase(0, trimmed.find_first_not_of("\t "));
      if (trimmed.compare(0, prefix.size(), prefix) == 0)
      {
        line = key + " = " + value;
        return true;
      }
    }

    lines.push_back(key + " = " + value);
    return true;
  }

  std::string formatHexValue(const DWORD value)
  {
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << value;
    return stream.str();
  }
}

std::vector<NexPad::MappingEntry> NexPad::getMappingEntries() const
{
  return {
      {"CONFIG_MOUSE_LEFT", "Mouse left click trigger", CONFIG_MOUSE_LEFT, false},
      {"CONFIG_MOUSE_RIGHT", "Mouse right click trigger", CONFIG_MOUSE_RIGHT, false},
      {"CONFIG_MOUSE_MIDDLE", "Mouse middle click trigger", CONFIG_MOUSE_MIDDLE, false},
      {"CONFIG_HIDE", "Toggle window visibility", CONFIG_HIDE, false},
      {"CONFIG_DISABLE", "Enable or disable NexPad", CONFIG_DISABLE, false},
      {"CONFIG_DISABLE_VIBRATION", "Toggle controller vibration", CONFIG_DISABLE_VIBRATION, false},
      {"CONFIG_SPEED_CHANGE", "Cycle through cursor speed presets", CONFIG_SPEED_CHANGE, false},
      {"CONFIG_OSK", "Toggle On-Screen Keyboard", CONFIG_OSK, false},
      {"GAMEPAD_DPAD_UP", "Keyboard mapping for D-Pad Up", GAMEPAD_DPAD_UP, true},
      {"GAMEPAD_DPAD_DOWN", "Keyboard mapping for D-Pad Down", GAMEPAD_DPAD_DOWN, true},
      {"GAMEPAD_DPAD_LEFT", "Keyboard mapping for D-Pad Left", GAMEPAD_DPAD_LEFT, true},
      {"GAMEPAD_DPAD_RIGHT", "Keyboard mapping for D-Pad Right", GAMEPAD_DPAD_RIGHT, true},
      {"GAMEPAD_START", "Keyboard mapping for Start / Options", GAMEPAD_START, true},
      {"GAMEPAD_BACK", "Keyboard mapping for Back / Share", GAMEPAD_BACK, true},
      {"GAMEPAD_LEFT_THUMB", "Keyboard mapping for Left Thumb", GAMEPAD_LEFT_THUMB, true},
      {"GAMEPAD_RIGHT_THUMB", "Keyboard mapping for Right Thumb", GAMEPAD_RIGHT_THUMB, true},
      {"GAMEPAD_LEFT_SHOULDER", "Keyboard mapping for Left Shoulder", GAMEPAD_LEFT_SHOULDER, true},
      {"GAMEPAD_RIGHT_SHOULDER", "Keyboard mapping for Right Shoulder", GAMEPAD_RIGHT_SHOULDER, true},
      {"GAMEPAD_A", "Keyboard mapping for A / Cross", GAMEPAD_A, true},
      {"GAMEPAD_B", "Keyboard mapping for B / Circle", GAMEPAD_B, true},
      {"GAMEPAD_X", "Keyboard mapping for X / Square", GAMEPAD_X, true},
      {"GAMEPAD_Y", "Keyboard mapping for Y / Triangle", GAMEPAD_Y, true},
      {"GAMEPAD_TRIGGER_LEFT", "Keyboard mapping for Left Trigger", GAMEPAD_TRIGGER_LEFT, true},
      {"GAMEPAD_TRIGGER_RIGHT", "Keyboard mapping for Right Trigger", GAMEPAD_TRIGGER_RIGHT, true},
      {"ON_ENABLE", "Key sent when NexPad becomes enabled", ON_ENABLE, true},
      {"ON_DISABLE", "Key sent when NexPad becomes disabled", ON_DISABLE, true}};
}

bool NexPad::setMappingValue(const std::string &key, DWORD value)
{
  if (key == "CONFIG_MOUSE_LEFT")
  {
    CONFIG_MOUSE_LEFT = value;
    return true;
  }
  if (key == "CONFIG_MOUSE_RIGHT")
  {
    CONFIG_MOUSE_RIGHT = value;
    return true;
  }
  if (key == "CONFIG_MOUSE_MIDDLE")
  {
    CONFIG_MOUSE_MIDDLE = value;
    return true;
  }
  if (key == "CONFIG_HIDE")
  {
    CONFIG_HIDE = value;
    return true;
  }
  if (key == "CONFIG_DISABLE")
  {
    CONFIG_DISABLE = value;
    return true;
  }
  if (key == "CONFIG_DISABLE_VIBRATION")
  {
    CONFIG_DISABLE_VIBRATION = value;
    return true;
  }
  if (key == "CONFIG_SPEED_CHANGE")
  {
    CONFIG_SPEED_CHANGE = value;
    return true;
  }
  if (key == "CONFIG_OSK")
  {
    CONFIG_OSK = value;
    return true;
  }
  if (key == "GAMEPAD_DPAD_UP")
  {
    GAMEPAD_DPAD_UP = value;
    return true;
  }
  if (key == "GAMEPAD_DPAD_DOWN")
  {
    GAMEPAD_DPAD_DOWN = value;
    return true;
  }
  if (key == "GAMEPAD_DPAD_LEFT")
  {
    GAMEPAD_DPAD_LEFT = value;
    return true;
  }
  if (key == "GAMEPAD_DPAD_RIGHT")
  {
    GAMEPAD_DPAD_RIGHT = value;
    return true;
  }
  if (key == "GAMEPAD_START")
  {
    GAMEPAD_START = value;
    return true;
  }
  if (key == "GAMEPAD_BACK")
  {
    GAMEPAD_BACK = value;
    return true;
  }
  if (key == "GAMEPAD_LEFT_THUMB")
  {
    GAMEPAD_LEFT_THUMB = value;
    return true;
  }
  if (key == "GAMEPAD_RIGHT_THUMB")
  {
    GAMEPAD_RIGHT_THUMB = value;
    return true;
  }
  if (key == "GAMEPAD_LEFT_SHOULDER")
  {
    GAMEPAD_LEFT_SHOULDER = value;
    return true;
  }
  if (key == "GAMEPAD_RIGHT_SHOULDER")
  {
    GAMEPAD_RIGHT_SHOULDER = value;
    return true;
  }
  if (key == "GAMEPAD_A")
  {
    GAMEPAD_A = value;
    return true;
  }
  if (key == "GAMEPAD_B")
  {
    GAMEPAD_B = value;
    return true;
  }
  if (key == "GAMEPAD_X")
  {
    GAMEPAD_X = value;
    return true;
  }
  if (key == "GAMEPAD_Y")
  {
    GAMEPAD_Y = value;
    return true;
  }
  if (key == "GAMEPAD_TRIGGER_LEFT")
  {
    GAMEPAD_TRIGGER_LEFT = value;
    return true;
  }
  if (key == "GAMEPAD_TRIGGER_RIGHT")
  {
    GAMEPAD_TRIGGER_RIGHT = value;
    return true;
  }
  if (key == "ON_ENABLE")
  {
    ON_ENABLE = value;
    return true;
  }
  if (key == "ON_DISABLE")
  {
    ON_DISABLE = value;
    return true;
  }
  return false;
}

bool NexPad::saveConfigFile()
{
  std::ifstream input(_configPath.c_str());
  if (!input)
  {
    notifyStatus("Unable to open config.ini for saving");
    return false;
  }

  std::vector<std::string> lines;
  for (std::string line; std::getline(input, line);)
  {
    lines.push_back(line);
  }
  input.close();

  std::ostringstream scrollValue;
  scrollValue << std::fixed << std::setprecision(3) << SCROLL_SPEED;

  updateConfigLine(lines, "CURRENT_SPEED_INDEX", std::to_string(speed_idx));
  updateConfigLine(lines, "SCROLL_SPEED", scrollValue.str());
  updateConfigLine(lines, "SWAP_THUMBSTICKS", std::to_string(SWAP_THUMBSTICKS));
  updateConfigLine(lines, "TOUCHPAD_ENABLED", std::to_string(TOUCHPAD_ENABLED));
  updateConfigLine(lines, "TOUCHPAD_DEAD_ZONE", std::to_string(TOUCHPAD_DEAD_ZONE));
  std::ostringstream touchpadSpeedValue;
  touchpadSpeedValue << std::fixed << std::setprecision(3) << TOUCHPAD_SPEED;
  updateConfigLine(lines, "TOUCHPAD_SPEED", touchpadSpeedValue.str());
  updateConfigLine(lines, "START_WITH_WINDOWS", std::to_string(START_WITH_WINDOWS));
  updateConfigLine(lines, "UI_THEME_MODE", std::to_string(UI_THEME_MODE));
  updateConfigLine(lines, "CONFIG_MOUSE_LEFT", formatHexValue(CONFIG_MOUSE_LEFT));
  updateConfigLine(lines, "CONFIG_MOUSE_RIGHT", formatHexValue(CONFIG_MOUSE_RIGHT));
  updateConfigLine(lines, "CONFIG_MOUSE_MIDDLE", formatHexValue(CONFIG_MOUSE_MIDDLE));
  updateConfigLine(lines, "CONFIG_HIDE", formatHexValue(CONFIG_HIDE));
  updateConfigLine(lines, "CONFIG_DISABLE", formatHexValue(CONFIG_DISABLE));
  updateConfigLine(lines, "CONFIG_DISABLE_VIBRATION", formatHexValue(CONFIG_DISABLE_VIBRATION));
  updateConfigLine(lines, "CONFIG_SPEED_CHANGE", formatHexValue(CONFIG_SPEED_CHANGE));
  updateConfigLine(lines, "CONFIG_OSK", formatHexValue(CONFIG_OSK));
  updateConfigLine(lines, "GAMEPAD_DPAD_UP", formatHexValue(GAMEPAD_DPAD_UP));
  updateConfigLine(lines, "GAMEPAD_DPAD_DOWN", formatHexValue(GAMEPAD_DPAD_DOWN));
  updateConfigLine(lines, "GAMEPAD_DPAD_LEFT", formatHexValue(GAMEPAD_DPAD_LEFT));
  updateConfigLine(lines, "GAMEPAD_DPAD_RIGHT", formatHexValue(GAMEPAD_DPAD_RIGHT));
  updateConfigLine(lines, "GAMEPAD_START", formatHexValue(GAMEPAD_START));
  updateConfigLine(lines, "GAMEPAD_BACK", formatHexValue(GAMEPAD_BACK));
  updateConfigLine(lines, "GAMEPAD_LEFT_THUMB", formatHexValue(GAMEPAD_LEFT_THUMB));
  updateConfigLine(lines, "GAMEPAD_RIGHT_THUMB", formatHexValue(GAMEPAD_RIGHT_THUMB));
  updateConfigLine(lines, "GAMEPAD_LEFT_SHOULDER", formatHexValue(GAMEPAD_LEFT_SHOULDER));
  updateConfigLine(lines, "GAMEPAD_RIGHT_SHOULDER", formatHexValue(GAMEPAD_RIGHT_SHOULDER));
  updateConfigLine(lines, "GAMEPAD_A", formatHexValue(GAMEPAD_A));
  updateConfigLine(lines, "GAMEPAD_B", formatHexValue(GAMEPAD_B));
  updateConfigLine(lines, "GAMEPAD_X", formatHexValue(GAMEPAD_X));
  updateConfigLine(lines, "GAMEPAD_Y", formatHexValue(GAMEPAD_Y));
  updateConfigLine(lines, "GAMEPAD_TRIGGER_LEFT", formatHexValue(GAMEPAD_TRIGGER_LEFT));
  updateConfigLine(lines, "GAMEPAD_TRIGGER_RIGHT", formatHexValue(GAMEPAD_TRIGGER_RIGHT));
  updateConfigLine(lines, "ON_ENABLE", formatHexValue(ON_ENABLE));
  updateConfigLine(lines, "ON_DISABLE", formatHexValue(ON_DISABLE));

  std::ofstream output(_configPath.c_str(), std::ios::trunc);
  if (!output)
  {
    notifyStatus("Unable to write config.ini");
    return false;
  }

  for (size_t index = 0; index < lines.size(); ++index)
  {
    output << lines[index];
    if (index + 1 < lines.size())
    {
      output << std::endl;
    }
  }

  notifyStatus("Saved settings to " + _configPath);
  return true;
}

std::string NexPad::getMappingsText() const
{
  std::ostringstream stream;
  appendHexSetting(stream, "CONFIG_MOUSE_LEFT", CONFIG_MOUSE_LEFT);
  appendHexSetting(stream, "CONFIG_MOUSE_RIGHT", CONFIG_MOUSE_RIGHT);
  appendHexSetting(stream, "CONFIG_MOUSE_MIDDLE", CONFIG_MOUSE_MIDDLE);
  appendHexSetting(stream, "CONFIG_HIDE", CONFIG_HIDE);
  appendHexSetting(stream, "CONFIG_DISABLE", CONFIG_DISABLE);
  appendHexSetting(stream, "CONFIG_DISABLE_VIBRATION", CONFIG_DISABLE_VIBRATION);
  appendHexSetting(stream, "CONFIG_SPEED_CHANGE", CONFIG_SPEED_CHANGE);
  appendHexSetting(stream, "CONFIG_OSK", CONFIG_OSK);
  appendHexSetting(stream, "GAMEPAD_DPAD_UP", GAMEPAD_DPAD_UP);
  appendHexSetting(stream, "GAMEPAD_DPAD_DOWN", GAMEPAD_DPAD_DOWN);
  appendHexSetting(stream, "GAMEPAD_DPAD_LEFT", GAMEPAD_DPAD_LEFT);
  appendHexSetting(stream, "GAMEPAD_DPAD_RIGHT", GAMEPAD_DPAD_RIGHT);
  appendHexSetting(stream, "GAMEPAD_START", GAMEPAD_START);
  appendHexSetting(stream, "GAMEPAD_BACK", GAMEPAD_BACK);
  appendHexSetting(stream, "GAMEPAD_LEFT_THUMB", GAMEPAD_LEFT_THUMB);
  appendHexSetting(stream, "GAMEPAD_RIGHT_THUMB", GAMEPAD_RIGHT_THUMB);
  appendHexSetting(stream, "GAMEPAD_LEFT_SHOULDER", GAMEPAD_LEFT_SHOULDER);
  appendHexSetting(stream, "GAMEPAD_RIGHT_SHOULDER", GAMEPAD_RIGHT_SHOULDER);
  appendHexSetting(stream, "GAMEPAD_A", GAMEPAD_A);
  appendHexSetting(stream, "GAMEPAD_B", GAMEPAD_B);
  appendHexSetting(stream, "GAMEPAD_X", GAMEPAD_X);
  appendHexSetting(stream, "GAMEPAD_Y", GAMEPAD_Y);
  appendHexSetting(stream, "GAMEPAD_TRIGGER_LEFT", GAMEPAD_TRIGGER_LEFT);
  appendHexSetting(stream, "GAMEPAD_TRIGGER_RIGHT", GAMEPAD_TRIGGER_RIGHT);
  appendHexSetting(stream, "ON_ENABLE", ON_ENABLE);
  appendHexSetting(stream, "ON_DISABLE", ON_DISABLE);
  return stream.str();
}

bool NexPad::applyMappingsText(const std::string &mappingsText)
{
  DWORD parsedValue = 0;

  if (!tryParseConfigValue(mappingsText, "CONFIG_MOUSE_LEFT", parsedValue))
    return false;
  CONFIG_MOUSE_LEFT = parsedValue;
  if (!tryParseConfigValue(mappingsText, "CONFIG_MOUSE_RIGHT", parsedValue))
    return false;
  CONFIG_MOUSE_RIGHT = parsedValue;
  if (!tryParseConfigValue(mappingsText, "CONFIG_MOUSE_MIDDLE", parsedValue))
    return false;
  CONFIG_MOUSE_MIDDLE = parsedValue;
  if (!tryParseConfigValue(mappingsText, "CONFIG_HIDE", parsedValue))
    return false;
  CONFIG_HIDE = parsedValue;
  if (!tryParseConfigValue(mappingsText, "CONFIG_DISABLE", parsedValue))
    return false;
  CONFIG_DISABLE = parsedValue;
  if (!tryParseConfigValue(mappingsText, "CONFIG_DISABLE_VIBRATION", parsedValue))
    return false;
  CONFIG_DISABLE_VIBRATION = parsedValue;
  if (!tryParseConfigValue(mappingsText, "CONFIG_SPEED_CHANGE", parsedValue))
    return false;
  CONFIG_SPEED_CHANGE = parsedValue;
  if (!tryParseConfigValue(mappingsText, "CONFIG_OSK", parsedValue))
    return false;
  CONFIG_OSK = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_DPAD_UP", parsedValue))
    return false;
  GAMEPAD_DPAD_UP = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_DPAD_DOWN", parsedValue))
    return false;
  GAMEPAD_DPAD_DOWN = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_DPAD_LEFT", parsedValue))
    return false;
  GAMEPAD_DPAD_LEFT = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_DPAD_RIGHT", parsedValue))
    return false;
  GAMEPAD_DPAD_RIGHT = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_START", parsedValue))
    return false;
  GAMEPAD_START = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_BACK", parsedValue))
    return false;
  GAMEPAD_BACK = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_LEFT_THUMB", parsedValue))
    return false;
  GAMEPAD_LEFT_THUMB = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_RIGHT_THUMB", parsedValue))
    return false;
  GAMEPAD_RIGHT_THUMB = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_LEFT_SHOULDER", parsedValue))
    return false;
  GAMEPAD_LEFT_SHOULDER = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_RIGHT_SHOULDER", parsedValue))
    return false;
  GAMEPAD_RIGHT_SHOULDER = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_A", parsedValue))
    return false;
  GAMEPAD_A = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_B", parsedValue))
    return false;
  GAMEPAD_B = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_X", parsedValue))
    return false;
  GAMEPAD_X = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_Y", parsedValue))
    return false;
  GAMEPAD_Y = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_TRIGGER_LEFT", parsedValue))
    return false;
  GAMEPAD_TRIGGER_LEFT = parsedValue;
  if (!tryParseConfigValue(mappingsText, "GAMEPAD_TRIGGER_RIGHT", parsedValue))
    return false;
  GAMEPAD_TRIGGER_RIGHT = parsedValue;
  if (!tryParseConfigValue(mappingsText, "ON_ENABLE", parsedValue))
    return false;
  ON_ENABLE = parsedValue;
  if (!tryParseConfigValue(mappingsText, "ON_DISABLE", parsedValue))
    return false;
  ON_DISABLE = parsedValue;

  notifyStatus("Applied button mappings from GUI");
  return true;
}

std::string NexPad::getProfileText() const
{
  std::ostringstream stream;
  stream << "# NexPad preset profile\r\n";
  stream << "CURRENT_SPEED_INDEX = " << speed_idx << "\r\n";
  stream << std::fixed << std::setprecision(3)
         << "SCROLL_SPEED = " << SCROLL_SPEED << "\r\n"
         << "TOUCHPAD_SPEED = " << TOUCHPAD_SPEED << "\r\n";
  stream << "SWAP_THUMBSTICKS = " << SWAP_THUMBSTICKS << "\r\n";
  stream << "TOUCHPAD_ENABLED = " << TOUCHPAD_ENABLED << "\r\n";
  stream << "TOUCHPAD_DEAD_ZONE = " << TOUCHPAD_DEAD_ZONE << "\r\n";
  stream << "UI_THEME_MODE = " << UI_THEME_MODE << "\r\n\r\n";
  stream << getMappingsText();
  return stream.str();
}

bool NexPad::applyProfileText(const std::string &profileText)
{
  if (!applyMappingsText(profileText))
  {
    return false;
  }

  DWORD parsedInteger = 0;
  if (tryParseConfigValue(profileText, "CURRENT_SPEED_INDEX", parsedInteger))
  {
    setSpeedIndex(static_cast<unsigned int>(parsedInteger));
  }

  float parsedScrollSpeed = 0.0f;
  if (tryParseFloatConfigValue(profileText, "SCROLL_SPEED", parsedScrollSpeed) && parsedScrollSpeed > 0.0f)
  {
    setScrollSpeed(parsedScrollSpeed);
  }

  if (tryParseConfigValue(profileText, "SWAP_THUMBSTICKS", parsedInteger))
  {
    setSwapThumbsticks(parsedInteger == 0 ? 0 : 1);
  }

  if (tryParseConfigValue(profileText, "TOUCHPAD_ENABLED", parsedInteger))
  {
    TOUCHPAD_ENABLED = parsedInteger == 0 ? 0 : 1;
  }

  if (tryParseConfigValue(profileText, "TOUCHPAD_DEAD_ZONE", parsedInteger))
  {
    TOUCHPAD_DEAD_ZONE = static_cast<int>(parsedInteger);
    if (TOUCHPAD_DEAD_ZONE < 0)
    {
      TOUCHPAD_DEAD_ZONE = 0;
    }
  }

  float parsedTouchpadSpeed = 0.0f;
  if (tryParseFloatConfigValue(profileText, "TOUCHPAD_SPEED", parsedTouchpadSpeed) && parsedTouchpadSpeed > 0.0f)
  {
    TOUCHPAD_SPEED = parsedTouchpadSpeed;
  }

  if (tryParseConfigValue(profileText, "UI_THEME_MODE", parsedInteger))
  {
    setUiThemeMode(static_cast<int>(parsedInteger));
  }

  notifyStatus("Imported preset profile");
  return true;
}

void NexPad::setDisabled(bool disabled)
{
  if (_disabled == disabled)
  {
    return;
  }

  int duration = 400;
  int intensity = 65000;
  _disabled = disabled;
  _touchpadTouchWasActive = false;
  _touchpadTapMoved = false;
  _touchpadTapStartTick = 0;

  if (_disabled)
  {
    duration = 400;
    intensity = 10000;
    releaseAllActiveInputs();

    if (ON_DISABLE)
    {
      inputKeyboardDown(static_cast<WORD>(ON_DISABLE));
      inputKeyboardUp(static_cast<WORD>(ON_DISABLE));
    }
    notifyStatus("NexPad Disabled");
  }
  else
  {
    if (ON_ENABLE)
    {
      inputKeyboardDown(static_cast<WORD>(ON_ENABLE));
      inputKeyboardUp(static_cast<WORD>(ON_ENABLE));
    }
    notifyStatus("NexPad Enabled");
  }

  pulseVibrate(duration, intensity, intensity);
}

void NexPad::processCurrentState()
{
  updateVibrationState();

  // Disable NexPad
  handleDisableButton();
  if (_disabled)
  {
    return;
  }

  // Vibration
  handleVibrationButton();

  updateTouchpadInteractionState();

  // Mouse functions
  handleMouseMovement();
  handleScrolling();
  handleTouchpadTapGesture();

  if (CONFIG_MOUSE_LEFT)
  {
    mapMouseClick(CONFIG_MOUSE_LEFT, MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP);
  }
  if (CONFIG_MOUSE_RIGHT)
  {
    mapMouseClick(CONFIG_MOUSE_RIGHT, MOUSEEVENTF_RIGHTDOWN, MOUSEEVENTF_RIGHTUP);
  }
  if (CONFIG_MOUSE_MIDDLE)
  {
    mapMouseClick(CONFIG_MOUSE_MIDDLE, MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_MIDDLEUP);
  }

  // Hides the console
  if (CONFIG_HIDE)
  {
    setXboxClickState(CONFIG_HIDE);
    if (_xboxClickIsDown[CONFIG_HIDE])
    {
      toggleWindowVisibility();
    }
  }

  // Toggle the on-screen keyboard
  if (CONFIG_OSK)
  {
    setXboxClickState(CONFIG_OSK);
    if (_xboxClickIsDown[CONFIG_OSK])
    {
      // Get the otk window
      HWND otk_win = getOskWindow();
      if (otk_win == NULL)
      {
        notifyStatus("Launching the On-Screen Keyboard");
        launchOsk();
      }
      else if (IsIconic(otk_win))
      {
        ShowWindow(otk_win, SW_RESTORE);
      }
      else
      {
        // Close Window
        PostMessage(otk_win, WM_CLOSE, 0, 0);
      }
    }
  }

  // Will change between the current speed values
  setXboxClickState(CONFIG_SPEED_CHANGE);
  if (_xboxClickIsDown[CONFIG_SPEED_CHANGE])
  {
    const int CHANGE_SPEED_VIBRATION_INTENSITY = 65000; // Speed of the vibration motors when changing cursor speed.
    const int CHANGE_SPEED_VIBRATION_DURATION = 450;    // Duration of the cursor speed change vibration in milliseconds.

    speed_idx++;
    if (speed_idx >= speeds.size())
    {
      speed_idx = 0;
    }
    speed = speeds[speed_idx];
    std::ostringstream speedMessage;
    speedMessage << "Setting speed to " << speed << " (" << speed_names[speed_idx] << ")";
    notifyStatus(speedMessage.str());
    pulseVibrate(CHANGE_SPEED_VIBRATION_DURATION, CHANGE_SPEED_VIBRATION_INTENSITY, CHANGE_SPEED_VIBRATION_INTENSITY);
  }

  // Update all controller keys.
  handleTriggers(static_cast<WORD>(GAMEPAD_TRIGGER_LEFT), static_cast<WORD>(GAMEPAD_TRIGGER_RIGHT));
  if (GAMEPAD_DPAD_UP)
  {
    mapKeyboard(XINPUT_GAMEPAD_DPAD_UP, static_cast<WORD>(GAMEPAD_DPAD_UP));
  }
  if (GAMEPAD_DPAD_DOWN)
  {
    mapKeyboard(XINPUT_GAMEPAD_DPAD_DOWN, static_cast<WORD>(GAMEPAD_DPAD_DOWN));
  }
  if (GAMEPAD_DPAD_LEFT)
  {
    mapKeyboard(XINPUT_GAMEPAD_DPAD_LEFT, static_cast<WORD>(GAMEPAD_DPAD_LEFT));
  }
  if (GAMEPAD_DPAD_RIGHT)
  {
    mapKeyboard(XINPUT_GAMEPAD_DPAD_RIGHT, static_cast<WORD>(GAMEPAD_DPAD_RIGHT));
  }
  if (GAMEPAD_START)
  {
    mapKeyboard(XINPUT_GAMEPAD_START, static_cast<WORD>(GAMEPAD_START));
  }
  if (GAMEPAD_BACK)
  {
    mapKeyboard(XINPUT_GAMEPAD_BACK, static_cast<WORD>(GAMEPAD_BACK));
  }
  if (GAMEPAD_LEFT_THUMB)
  {
    mapKeyboard(XINPUT_GAMEPAD_LEFT_THUMB, static_cast<WORD>(GAMEPAD_LEFT_THUMB));
  }
  if (GAMEPAD_RIGHT_THUMB)
  {
    mapKeyboard(XINPUT_GAMEPAD_RIGHT_THUMB, static_cast<WORD>(GAMEPAD_RIGHT_THUMB));
  }
  if (GAMEPAD_LEFT_SHOULDER)
  {
    mapKeyboard(XINPUT_GAMEPAD_LEFT_SHOULDER, static_cast<WORD>(GAMEPAD_LEFT_SHOULDER));
  }
  if (GAMEPAD_RIGHT_SHOULDER)
  {
    mapKeyboard(XINPUT_GAMEPAD_RIGHT_SHOULDER, static_cast<WORD>(GAMEPAD_RIGHT_SHOULDER));
  }
  if (GAMEPAD_A)
  {
    mapKeyboard(XINPUT_GAMEPAD_A, static_cast<WORD>(GAMEPAD_A));
  }
  if (GAMEPAD_B)
  {
    mapKeyboard(XINPUT_GAMEPAD_B, static_cast<WORD>(GAMEPAD_B));
  }
  if (GAMEPAD_X)
  {
    mapKeyboard(XINPUT_GAMEPAD_X, static_cast<WORD>(GAMEPAD_X));
  }
  if (GAMEPAD_Y)
  {
    mapKeyboard(XINPUT_GAMEPAD_Y, static_cast<WORD>(GAMEPAD_Y));
  }
}

void NexPad::updateTouchpadInteractionState()
{
  const CXBOXController::TouchpadState touchpad = _controller->GetTouchpadState();
  if (TOUCHPAD_ENABLED == 0 || !touchpad.available)
  {
    resetTouchpadInteractionState();
    return;
  }

  if (_touchpadAwaitingReleaseAfterScroll)
  {
    if (!touchpad.active)
    {
      _touchpadAwaitingReleaseAfterScroll = false;
      _touchpadInteractionMode = TouchpadInteractionMode::Idle;
    }
    else
    {
      _touchpadInteractionMode = TouchpadInteractionMode::Idle;
      return;
    }
  }

  if (touchpad.reliableTwoFinger && touchpad.activeFingerCount >= 2)
  {
    _touchpadInteractionMode = TouchpadInteractionMode::TwoFingerScroll;
    _touchpadTouchWasActive = false;
    _touchpadTapMoved = false;
    _touchpadTapStartTick = 0;
    return;
  }

  if (_touchpadInteractionMode == TouchpadInteractionMode::TwoFingerScroll)
  {
    _touchpadInteractionMode = TouchpadInteractionMode::Idle;
    _touchpadAwaitingReleaseAfterScroll = touchpad.active;
    _touchpadTouchWasActive = false;
    _touchpadTapMoved = false;
    _touchpadTapStartTick = 0;
    _touchpadScrollXRest = 0.0f;
    _touchpadScrollYRest = 0.0f;
    if (_touchpadAwaitingReleaseAfterScroll)
    {
      return;
    }
  }

  if (touchpad.active && touchpad.activeFingerCount == 1)
  {
    _touchpadInteractionMode = TouchpadInteractionMode::OneFingerCursor;
    return;
  }

  _touchpadInteractionMode = TouchpadInteractionMode::Idle;
}

// Description:
//   Sends a vibration pulse to the controller for a duration of time.
//     This is a BLOCKING call. Any inputs during the vibration will be IGNORED.
//
// Params:
//   duration   The length of time in milliseconds to vibrate for
//   l          The speed (intensity) of the left vibration motor
//   r          The speed (intensity) of the right vibration motor
void NexPad::pulseVibrate(const int duration, const int l, const int r)
{
  startVibrationPulse(duration, l, r);
}

void NexPad::stopVibration()
{
  if (_controller != NULL)
  {
    _controller->Vibrate(0, 0);
  }

  _vibrationActive = false;
  _vibrationLeftMotor = 0;
  _vibrationRightMotor = 0;
  _vibrationEndTick = 0;
}

// Description:
//   Toggles the controller mapping after checking for the disable configuration command.
void NexPad::handleDisableButton()
{
  setXboxClickState(CONFIG_DISABLE);
  if (_xboxClickIsDown[CONFIG_DISABLE])
  {
    setDisabled(!_disabled);
  }
}

// Description:
//   Toggles the vibration support after checking for the diable vibration command.
//   This function will BLOCK to prevent rapidly toggling the vibration.
void NexPad::handleVibrationButton()
{
  setXboxClickState(CONFIG_DISABLE_VIBRATION);
  const DWORD now = GetTickCount();
  if (_xboxClickIsDown[CONFIG_DISABLE_VIBRATION] && !isVibrationToggleCoolingDown(now))
  {
    _vibrationDisabled = !_vibrationDisabled;
    _vibrationToggleCooldownUntilTick = now + 1000;
    if (_vibrationDisabled)
    {
      stopVibration();
    }
    notifyStatus(std::string("Vibration ") + (_vibrationDisabled ? "Disabled" : "Enabled"));
  }
}

// Description:
//   Toggles the visibility of the window.
void NexPad::toggleWindowVisibility()
{
  _hidden = !_hidden;
  notifyStatus(std::string("Window ") + (_hidden ? "hidden" : "shown"));
  setWindowVisibility(_hidden);
}

// Description:
//   Either hides or shows the window.
//
// Params:
//   hidden   Hides the window when true
void NexPad::setWindowVisibility(const bool &hidden) const
{
  HWND hWnd = _windowHandle != NULL ? _windowHandle : GetConsoleWindow();
  if (hWnd != NULL)
  {
    ShowWindow(hWnd, hidden ? SW_HIDE : SW_SHOW);
  }
}

template <typename T>
int sgn(T val)
{
  return (T(0) < val) - (val < T(0));
}

// Description:
//   Determines if the thumbstick value is valid and converts it to a float.
//
// Params:
//   t  Analog thumbstick value to check and convert
//
// Returns:
//   If the value is valid, t will be returned as-is as a float. If the value is
//     invalid, 0 will be returned.
float NexPad::getDelta(short t)
{
  // filter non-32768 and 32767, wireless ones can glitch sometimes and send it to the edge of the screen, it'll toss out some HUGE integer even when it's centered
  if (t > 32767)
    t = 0;
  if (t < -32768)
    t = 0;

  return t;
}

// Description:
//   Calculates a multiplier for an analog thumbstick based on the update rate.
//
// Params:
//   magnitude The thumbstick magnitude in XY-plane, which is sqrt(deltaX*deltaX + deltaY*deltaY). Must be larger than deadzone.
//   deadzone   The dead zone to use for this thumbstick
//   accel      An exponent to use to create an input curve (Optional). 0 to use a linear input
//
// Returns:
//   Multiplier used to properly scale the given thumbstick value.
float NexPad::getMult(float magnitude, float deadzone, float accel = 0.0f)
{
  // Normalize the thumbstick value (result is in range 0 to 1).
  // Note that the smallest accepted thumbstick distance is deadzone.
  // Thus, 0% would be deadzone (and below) and 100% would be (MAXSHORT - deadzone)
  float mult = (magnitude - deadzone) / (MAXSHORT - deadzone);

  // Apply a curve to the normalized thumbstick value.
  if (accel > 0.0001f)
  {
    mult = pow(mult, accel);
  }
  return mult / FPS;
}

void NexPad::handleTouchpadMovement(float &dx, float &dy)
{
  const float TOUCHPAD_TAP_CANCEL_CURSOR_TRAVEL = 1.5f;
  const float TOUCHPAD_CURSOR_RESPONSE_RANGE = 24.0f;
  const float TOUCHPAD_CURSOR_MIN_RESPONSE = 0.10f;

  if (TOUCHPAD_ENABLED == 0)
  {
    return;
  }

  const CXBOXController::TouchpadState touchpad = _controller->GetTouchpadState();
  if (_touchpadInteractionMode != TouchpadInteractionMode::OneFingerCursor ||
      !touchpad.available ||
      !touchpad.active ||
      touchpad.activeFingerCount != 1)
  {
    return;
  }

  const int magnitudeX = std::abs(static_cast<int>(touchpad.deltaX));
  const int magnitudeY = std::abs(static_cast<int>(touchpad.deltaY));
  if (magnitudeX <= TOUCHPAD_DEAD_ZONE && magnitudeY <= TOUCHPAD_DEAD_ZONE)
  {
    return;
  }

  const float touchpadDeltaX = static_cast<float>(touchpad.deltaX);
  const float touchpadDeltaY = static_cast<float>(touchpad.deltaY);
  const float movementMagnitude = std::sqrt((touchpadDeltaX * touchpadDeltaX) + (touchpadDeltaY * touchpadDeltaY));
  if (movementMagnitude <= static_cast<float>(TOUCHPAD_DEAD_ZONE))
  {
    return;
  }

  const float adjustedMagnitude = movementMagnitude - static_cast<float>(TOUCHPAD_DEAD_ZONE);
  const float movementScale = adjustedMagnitude / movementMagnitude;
  float normalizedMagnitude = adjustedMagnitude / TOUCHPAD_CURSOR_RESPONSE_RANGE;
  if (normalizedMagnitude > 1.0f)
  {
    normalizedMagnitude = 1.0f;
  }

  const float precisionResponse = TOUCHPAD_CURSOR_MIN_RESPONSE +
                                  ((1.0f - TOUCHPAD_CURSOR_MIN_RESPONSE) * normalizedMagnitude * normalizedMagnitude);
  const float cursorScale = movementScale * precisionResponse * TOUCHPAD_SPEED;
  const float cursorDeltaX = touchpadDeltaX * cursorScale;
  const float cursorDeltaY = touchpadDeltaY * cursorScale;

  _touchpadTapCursorTravelX += cursorDeltaX;
  _touchpadTapCursorTravelY += cursorDeltaY;
  if (std::fabs(_touchpadTapCursorTravelX) > TOUCHPAD_TAP_CANCEL_CURSOR_TRAVEL ||
      std::fabs(_touchpadTapCursorTravelY) > TOUCHPAD_TAP_CANCEL_CURSOR_TRAVEL)
  {
    _touchpadTapMoved = true;
  }

  dx += cursorDeltaX;
  dy -= cursorDeltaY;
}

void NexPad::handleTouchpadScrolling()
{
  if (TOUCHPAD_ENABLED == 0)
  {
    _touchpadScrollRawX = 0.0f;
    _touchpadScrollRawY = 0.0f;
    _touchpadScrollXRest = 0.0f;
    _touchpadScrollYRest = 0.0f;
    return;
  }

  const CXBOXController::TouchpadState touchpad = _controller->GetTouchpadState();
  if (_touchpadInteractionMode != TouchpadInteractionMode::TwoFingerScroll ||
      !touchpad.available ||
      !touchpad.active ||
      !touchpad.reliableTwoFinger ||
      touchpad.activeFingerCount < 2)
  {
    _touchpadScrollRawX = 0.0f;
    _touchpadScrollRawY = 0.0f;
    _touchpadScrollXRest = 0.0f;
    _touchpadScrollYRest = 0.0f;
    return;
  }

  short filteredDeltaX = touchpad.scrollDeltaX;
  short filteredDeltaY = touchpad.scrollDeltaY;
  const int magnitudeX = std::abs(static_cast<int>(filteredDeltaX));
  const int magnitudeY = std::abs(static_cast<int>(filteredDeltaY));

  if (magnitudeX == 0 && magnitudeY == 0)
  {
    return;
  }

  if (magnitudeX > magnitudeY * 2)
  {
    filteredDeltaY = 0;
  }
  else if (magnitudeY > magnitudeX * 2)
  {
    filteredDeltaX = 0;
  }

  _touchpadScrollRawX += static_cast<float>(filteredDeltaX);
  _touchpadScrollRawY += static_cast<float>(filteredDeltaY);

  const float TOUCHPAD_SCROLL_MOTION_THRESHOLD = 1.0f;
  if (std::fabs(_touchpadScrollRawX) < TOUCHPAD_SCROLL_MOTION_THRESHOLD &&
      std::fabs(_touchpadScrollRawY) < TOUCHPAD_SCROLL_MOTION_THRESHOLD)
  {
    return;
  }

  const float TOUCHPAD_SCROLL_SCALE = 96.0f;
  const float scrollXValue = _touchpadScrollXRest + (_touchpadScrollRawX * SCROLL_SPEED * TOUCHPAD_SCROLL_SCALE);
  const float scrollYValue = _touchpadScrollYRest - (_touchpadScrollRawY * SCROLL_SPEED * TOUCHPAD_SCROLL_SCALE);

  const int scrollX = static_cast<int>(scrollXValue);
  const int scrollY = static_cast<int>(scrollYValue);

  _touchpadScrollXRest = scrollXValue - static_cast<float>(scrollX);
  _touchpadScrollYRest = scrollYValue - static_cast<float>(scrollY);
  _touchpadScrollRawX = 0.0f;
  _touchpadScrollRawY = 0.0f;

  if (scrollX != 0)
  {
    mouseEvent(MOUSEEVENTF_HWHEEL, static_cast<DWORD>(scrollX));
  }
  if (scrollY != 0)
  {
    mouseEvent(MOUSEEVENTF_WHEEL, static_cast<DWORD>(scrollY));
  }
}

void NexPad::handleTouchpadTapGesture()
{
  const DWORD TOUCHPAD_TAP_MAX_DURATION_MS = 350;
  const CXBOXController::TouchpadState touchpad = _controller->GetTouchpadState();

  if (TOUCHPAD_ENABLED == 0 || !touchpad.available)
  {
    resetTouchpadInteractionState();
    return;
  }

  if (_touchpadInteractionMode == TouchpadInteractionMode::TwoFingerScroll || _touchpadAwaitingReleaseAfterScroll)
  {
    _touchpadTouchWasActive = false;
    _touchpadTapMoved = false;
    _touchpadTapStartTick = 0;
    return;
  }

  if (touchpad.active && touchpad.activeFingerCount == 1)
  {
    if (!_touchpadTouchWasActive)
    {
      _touchpadTouchWasActive = true;
      _touchpadTapMoved = false;
      _touchpadTapStartTick = GetTickCount();
      _touchpadTapCursorTravelX = 0.0f;
      _touchpadTapCursorTravelY = 0.0f;
    }

    return;
  }

  if (!_touchpadTouchWasActive)
  {
    return;
  }

  const DWORD tapDuration = GetTickCount() - _touchpadTapStartTick;
  if (!_touchpadTapMoved && tapDuration <= TOUCHPAD_TAP_MAX_DURATION_MS)
  {
    mouseEvent(MOUSEEVENTF_LEFTDOWN);
    mouseEvent(MOUSEEVENTF_LEFTUP);
  }

  _touchpadTouchWasActive = false;
  _touchpadTapMoved = false;
  _touchpadTapStartTick = 0;
  _touchpadTapCursorTravelX = 0.0f;
  _touchpadTapCursorTravelY = 0.0f;
}

// Description:
//   Controls the mouse cursor movement by reading the left thumbstick.
void NexPad::handleMouseMovement()
{
  POINT cursor;
  GetCursorPos(&cursor);

  float tx;
  float ty;

  if (SWAP_THUMBSTICKS == 0)
  {
    // Use left stick
    tx = getDelta(_currentState.Gamepad.sThumbLX);
    ty = getDelta(_currentState.Gamepad.sThumbLY);
  }
  else
  {
    // Use right stick
    tx = getDelta(_currentState.Gamepad.sThumbRX);
    ty = getDelta(_currentState.Gamepad.sThumbRY);
  }

  float x = cursor.x + _xRest;
  float y = cursor.y + _yRest;

  float dx = 0;
  float dy = 0;

  // Handle dead zone
  const float lengthsq = tx * tx + ty * ty;
  const float deadZone = static_cast<float>(DEAD_ZONE);
  if (lengthsq > deadZone * deadZone)
  {
    const float mult = speed * getMult(std::sqrt(lengthsq), deadZone, acceleration_factor);

    dx = tx * mult;
    dy = ty * mult;
  }

  handleTouchpadMovement(dx, dy);

  x += dx;
  _xRest = x - (float)((int)x);

  y -= dy;
  _yRest = y - (float)((int)y);

  SetCursorPos((int)x, (int)y); // after all click input processing
}

// Description:
//   Controls the scroll wheel movement by reading the right thumbstick.
void NexPad::handleScrolling()
{
  float tx;
  float ty;

  if (SWAP_THUMBSTICKS == 0)
  {
    // Use right stick
    tx = getDelta(_currentState.Gamepad.sThumbRX);
    ty = getDelta(_currentState.Gamepad.sThumbRY);
  }
  else
  {
    // Use left stick
    tx = getDelta(_currentState.Gamepad.sThumbLX);
    ty = getDelta(_currentState.Gamepad.sThumbLY);
  }

  // Handle dead zone
  float magnitudeX = std::fabs(tx);
  float magnitudeY = std::fabs(ty);
  if (magnitudeX > SCROLL_DEAD_ZONE)
  {
    int scrollX = static_cast<int>(tx * getMult(magnitudeX, static_cast<float>(SCROLL_DEAD_ZONE)) * SCROLL_SPEED);
    mouseEvent(MOUSEEVENTF_HWHEEL, static_cast<DWORD>(scrollX));
  }
  if (magnitudeY > SCROLL_DEAD_ZONE)
  {
    int scrollY = static_cast<int>(ty * getMult(magnitudeY, static_cast<float>(SCROLL_DEAD_ZONE)) * SCROLL_SPEED);
    mouseEvent(MOUSEEVENTF_WHEEL, static_cast<DWORD>(scrollY));
  }

  handleTouchpadScrolling();
}

// Description:
//   Handles the trigger-to-key mapping. The triggers are handled separately since
//     they are analog instead of a simple button press.
//
// Params:
//   lKey   The mapped key for the left trigger
//   rKey   The mapped key for the right trigger
void NexPad::handleTriggers(WORD lKey, WORD rKey)
{
  bool lTriggerIsDown = _currentState.Gamepad.bLeftTrigger > TRIGGER_DEAD_ZONE;
  bool rTriggerIsDown = _currentState.Gamepad.bRightTrigger > TRIGGER_DEAD_ZONE;

  // Handle left trigger
  if (lTriggerIsDown != _lTriggerPrevious)
  {
    _lTriggerPrevious = lTriggerIsDown;
    if (lTriggerIsDown)
    {
      inputKeyboardDown(lKey);
    }
    else
    {
      inputKeyboardUp(lKey);
    }
  }

  // Handle right trigger
  if (rTriggerIsDown != _rTriggerPrevious)
  {
    _rTriggerPrevious = rTriggerIsDown;
    if (rTriggerIsDown)
    {
      inputKeyboardDown(rKey);
    }
    else
    {
      inputKeyboardUp(rKey);
    }
  }
}

// Description:
//   Handles the state of a controller button press.
//
// Params:
//   STATE  The NexPad state, or command, to update
void NexPad::setXboxClickState(DWORD STATE)
{
  _xboxClickIsDown[STATE] = false;
  _xboxClickIsUp[STATE] = false;

  if (!this->xboxClickStateExists(STATE))
  {
    _xboxClickStateLastIteration[STATE] = false;
  }

  bool isDown = (_currentState.Gamepad.wButtons & STATE) == STATE;

  // Detect if the button has been pressed.
  if (isDown && !_xboxClickStateLastIteration[STATE])
  {
    _xboxClickStateLastIteration[STATE] = true;
    _xboxClickIsDown[STATE] = true;
    _xboxClickDownLength[STATE] = 0;
    _xboxClickIsDownLong[STATE] = false;
  }

  // Detect if the button has been held as a long press.
  if (isDown && _xboxClickStateLastIteration[STATE])
  {
    const int LONG_PRESS_TIME = 200; // milliseconds

    ++_xboxClickDownLength[STATE];
    if (_xboxClickDownLength[STATE] * SLEEP_AMOUNT > LONG_PRESS_TIME)
    {
      _xboxClickIsDownLong[STATE] = true;
    }
  }

  // Detect if the button has been released.
  if (!isDown && _xboxClickStateLastIteration[STATE])
  {
    _xboxClickStateLastIteration[STATE] = false;
    _xboxClickIsUp[STATE] = true;
    _xboxClickIsDownLong[STATE] = false;
  }

  _xboxClickStateLastIteration[STATE] = isDown;
}

// Description:
//   Check to see if a controller state exists in NexPad's button map.
//
// Params:
//   xinput   The NexPad state, or command, to search for
//
// Returns:
//   true if the state is present in the map.
bool NexPad::xboxClickStateExists(DWORD STATE)
{
  auto it = _xboxClickStateLastIteration.find(STATE);
  if (it == _xboxClickStateLastIteration.end())
  {
    return false;
  }

  return true;
}

// Description:
//   Presses or releases a key based on a mapped NexPad state.
//
// Params:
//   STATE  The NexPad state, or command, to trigger a key event
//   key    The key value to input to the system
void NexPad::mapKeyboard(DWORD STATE, WORD key)
{
  setXboxClickState(STATE);
  if (_xboxClickIsDown[STATE])
  {
    inputKeyboardDown(key);

    // Add key to the list of pressed keys.
    _pressedKeys.push_back(key);
  }

  if (_xboxClickIsUp[STATE])
  {
    inputKeyboardUp(key);

    // Remove key from the list of pressed keys.
    erasePressedKey(key);
  }
}

// Description:
//   Presses or releases a mouse button based on a mapped NexPad state
//
// Params:
//   STATE    The NexPad state, or command, to trigger a mouse event
//   keyDown  The button down event for a mouse event
//   keyUp    The button up event for a mouse event
void NexPad::mapMouseClick(DWORD STATE, DWORD keyDown, DWORD keyUp)
{
  setXboxClickState(STATE);
  if (_xboxClickIsDown[STATE])
  {
    mouseEvent(keyDown);

    // Add key to the list of pressed keys.
    if (keyDown == MOUSEEVENTF_LEFTDOWN)
    {
      _pressedKeys.push_back(VK_LBUTTON);
    }
    else if (keyDown == MOUSEEVENTF_RIGHTDOWN)
    {
      _pressedKeys.push_back(VK_RBUTTON);
    }
    else if (keyDown == MOUSEEVENTF_MIDDLEDOWN)
    {
      _pressedKeys.push_back(VK_MBUTTON);
    }
  }

  if (_xboxClickIsUp[STATE])
  {
    mouseEvent(keyUp);

    // Remove key from the list of pressed keys.
    if (keyUp == MOUSEEVENTF_LEFTUP)
    {
      erasePressedKey(VK_LBUTTON);
    }
    else if (keyUp == MOUSEEVENTF_RIGHTUP)
    {
      erasePressedKey(VK_RBUTTON);
    }
    else if (keyUp == MOUSEEVENTF_MIDDLEUP)
    {
      erasePressedKey(VK_MBUTTON);
    }
  }

  /*if (_xboxClickIsDownLong[STATE])
  {
    mouseEvent(keyDown | keyUp);
    mouseEvent(keyDown | keyUp);
  }*/
}

void NexPad::releaseAllActiveInputs()
{
  while (_pressedKeys.size() != 0)
  {
    std::list<WORD>::iterator it = _pressedKeys.begin();

    if (*it == VK_LBUTTON)
    {
      mouseEvent(MOUSEEVENTF_LEFTUP);
    }
    else if (*it == VK_RBUTTON)
    {
      mouseEvent(MOUSEEVENTF_RIGHTUP);
    }
    else if (*it == VK_MBUTTON)
    {
      mouseEvent(MOUSEEVENTF_MIDDLEUP);
    }
    else
    {
      inputKeyboardUp(*it);
    }

    _pressedKeys.erase(it);
  }

  _lTriggerPrevious = false;
  _rTriggerPrevious = false;
  resetTouchpadInteractionState();
  _xboxClickStateLastIteration.clear();
  _xboxClickIsDown.clear();
  _xboxClickIsDownLong.clear();
  _xboxClickDownLength.clear();
  _xboxClickIsUp.clear();
  stopVibration();
}

void NexPad::onControllerDisconnected()
{
  releaseAllActiveInputs();
}

void NexPad::resetTouchpadInteractionState()
{
  _touchpadTouchWasActive = false;
  _touchpadTapMoved = false;
  _touchpadAwaitingReleaseAfterScroll = false;
  _touchpadTapStartTick = 0;
  _touchpadTapCursorTravelX = 0.0f;
  _touchpadTapCursorTravelY = 0.0f;
  _touchpadInteractionMode = TouchpadInteractionMode::Idle;
  _touchpadScrollRawX = 0.0f;
  _touchpadScrollRawY = 0.0f;
  _touchpadScrollXRest = 0.0f;
  _touchpadScrollYRest = 0.0f;
}

void NexPad::updateVibrationState()
{
  if (_vibrationDisabled)
  {
    if (_vibrationActive)
    {
      stopVibration();
    }
    return;
  }

  if (!_vibrationActive)
  {
    return;
  }

  const DWORD now = GetTickCount();
  if (hasTickElapsed(now, _vibrationEndTick))
  {
    stopVibration();
  }
}

void NexPad::startVibrationPulse(int durationMs, int leftMotor, int rightMotor)
{
  if (_vibrationDisabled || _controller == NULL)
  {
    return;
  }

  _vibrationLeftMotor = leftMotor;
  _vibrationRightMotor = rightMotor;
  _vibrationEndTick = GetTickCount() + static_cast<DWORD>(durationMs < 0 ? 0 : durationMs);
  _vibrationActive = true;
  _controller->Vibrate(_vibrationLeftMotor, _vibrationRightMotor);
}

bool NexPad::isVibrationToggleCoolingDown(const DWORD now) const
{
  if (_vibrationToggleCooldownUntilTick == 0)
  {
    return false;
  }

  return !hasTickElapsed(now, _vibrationToggleCooldownUntilTick);
}

// Description:
//   Callback function used for the EnumWindows call to determine if we
//     have found the On-Screen Keyboard window.
//
// Params:
//   curWnd   The current window to check
//   lParam   A callback parameter used to store the window if it is found
//
// Returns:
//   FALSE when the the desired window is found.
static BOOL CALLBACK EnumWindowsProc(HWND curWnd, LPARAM lParam)
{
  TCHAR title[256];
  // Check to see if the window title matches what we are looking for.
  if (GetWindowText(curWnd, title, 256) && !_tcscmp(title, _T("On-Screen Keyboard")))
  {
    *(HWND *)lParam = curWnd;
    return FALSE; // Correct window found, stop enumerating through windows.
  }

  return TRUE;
}

// Description:
//   Finds the On-Screen Keyboard if it is open.
//
// Returns:
//   If found, the handle to the On-Screen Keyboard handle. Otherwise, returns NULL.
HWND NexPad::getOskWindow()
{
  HWND ret = NULL;
  EnumWindows(EnumWindowsProc, (LPARAM)&ret);
  return ret;
}

// Description:
//   Launches the On-Screen-Keyboard
void NexPad::launchOsk()
{
  PVOID OldValue = NULL;
  Wow64DisableWow64FsRedirection(&OldValue);
  ShellExecute((HWND)NULL, L"open", L"osk.exe", NULL, NULL, SW_SHOWNORMAL);
}

// Description:
//   Removes an entry for a pressed key from the list.
//
// Params:
//   key  The key value to remove from the pressed key list.
//
// Returns:
//   True if the given key was found and removed from the list.
bool NexPad::erasePressedKey(WORD key)
{
  for (std::list<WORD>::iterator it = _pressedKeys.begin();
       it != _pressedKeys.end();
       ++it)
  {
    if (*it == key)
    {
      _pressedKeys.erase(it);
      return true;
    }
  }

  return false;
}
