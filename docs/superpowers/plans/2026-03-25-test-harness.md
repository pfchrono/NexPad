# NexPad Automated Test Harness Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Wire Google Test into the NexPad solution so `ConfigFile` parsing, DualSense HID parsing, and `NexPad` button state logic are regression-tested in CI without real hardware.

**Architecture:** Google Test via git submodule at repo root (`googletest/`). A new VS project `NexPadTests` in the existing solution compiles `CXBOXController.cpp`, `ConfigFile.cpp`, and `NexPad.cpp` directly — no Win32 UI entry point (`main.cpp` is excluded). DualSense HID parsers are promoted from anonymous namespace to `NexPadInternal` namespace so tests can call them. Three accessor methods are added to `NexPad` for button state inspection.

**Tech Stack:** MSVC v143, Google Test (git submodule, source build), MSBuild, GitHub Actions (windows-2022 runner)

---

## File Map

| File | Action | Responsibility |
|------|--------|----------------|
| `googletest/` | Create (submodule) | Google Test framework source |
| `Windows/NexPadTests/NexPadTests.vcxproj` | Create | Test project — compiles source + gtest + test files |
| `Windows/NexPad.sln` | Modify | Add NexPadTests project entry |
| `Windows/NexPad/CXBOXController.h` | Modify | Add `#include <vector>` + `NexPadInternal` forward declarations |
| `Windows/NexPad/CXBOXController.cpp` | Modify | Move 4 parser fns from anonymous → `NexPadInternal` namespace |
| `Windows/NexPad/NexPad.h` | Modify | Add 3 button state accessor declarations |
| `Windows/NexPad/NexPad.cpp` | Modify | Implement 3 accessors |
| `Windows/NexPadTests/test_configfile.cpp` | Create | `ConfigFile` + `Convert` tests |
| `Windows/NexPadTests/test_controller_parsing.cpp` | Create | DualSense HID parser tests |
| `Windows/NexPadTests/test_button_state.cpp` | Create | `NexPad` button edge detection tests |
| `.github/workflows/build.yml` | Modify | Add `submodules: recursive`; add test run step |

---

## Task 1: Add googletest submodule

**Files:**
- Create: `googletest/` (git submodule)

- [ ] **Step 1: Add the submodule**

```bash
cd F:/code/NexPad
git submodule add https://github.com/google/googletest googletest
```

Expected: `googletest/` directory created, `.gitmodules` file created/updated.

- [ ] **Step 2: Verify the submodule fetched**

```bash
ls googletest/googletest/src/gtest-all.cc
```

Expected: file exists.

- [ ] **Step 3: Commit**

```bash
git add .gitmodules googletest
git commit -m "chore: add googletest as git submodule"
```

---

## Task 2: Promote HID parsers to `NexPadInternal` namespace

**Files:**
- Modify: `Windows/NexPad/CXBOXController.h`
- Modify: `Windows/NexPad/CXBOXController.cpp`

**Context:** `CXBOXController.cpp` has an anonymous namespace containing many helpers (`toThumbAxis`, `mapDualSenseButtons`, etc.) and four parser functions that need to be testable:
- `parseDualSenseUsbReport`
- `parseDualSenseBluetoothSimpleReport`
- `parseDualSenseBluetoothEnhancedReport`
- `parseDualSenseReport` (dispatcher)

The helpers stay in the anonymous namespace; only these four move. Functions in `namespace NexPadInternal {}` defined in the same `.cpp` file can still call anonymous-namespace helpers — no linkage problem.

- [ ] **Step 1: Add `#include <vector>` and `NexPadInternal` declarations to `CXBOXController.h`**

In `Windows/NexPad/CXBOXController.h`, after the existing includes (`<windows.h>`, `<xinput.h>`, `<string>`), add:

```cpp
#include <vector>

namespace NexPadInternal {
  bool parseDualSenseUsbReport(const std::vector<BYTE>& report, XINPUT_STATE* state);
  bool parseDualSenseBluetoothSimpleReport(const std::vector<BYTE>& report, XINPUT_STATE* state);
  bool parseDualSenseBluetoothEnhancedReport(const std::vector<BYTE>& report, XINPUT_STATE* state);
  bool parseDualSenseReport(const std::vector<BYTE>& report, XINPUT_STATE* state);
}
```

Place these declarations after the `#include` block and before the `class CXBOXController` definition.

- [ ] **Step 2: Move the four parser functions from anonymous namespace to `NexPadInternal` in `CXBOXController.cpp`**

In `CXBOXController.cpp`, locate the anonymous namespace block containing the four functions. Extract them:

```cpp
// BEFORE (inside anonymous namespace):
namespace {
  // ... helpers stay here (toThumbAxis, mapDualSenseButtons, etc.) ...

  bool parseDualSenseUsbReport(const std::vector<BYTE> &report, XINPUT_STATE *state) { ... }
  bool parseDualSenseBluetoothSimpleReport(const std::vector<BYTE> &report, XINPUT_STATE *state) { ... }
  bool parseDualSenseBluetoothEnhancedReport(const std::vector<BYTE> &report, XINPUT_STATE *state) { ... }
  bool parseDualSenseReport(const std::vector<BYTE> &report, XINPUT_STATE *state) { ... }
} // end anonymous namespace

// AFTER: remove the four functions from anonymous namespace, add:
namespace NexPadInternal {
  bool parseDualSenseUsbReport(const std::vector<BYTE> &report, XINPUT_STATE *state) { /* same body */ }
  bool parseDualSenseBluetoothSimpleReport(const std::vector<BYTE> &report, XINPUT_STATE *state) { /* same body */ }
  bool parseDualSenseBluetoothEnhancedReport(const std::vector<BYTE> &report, XINPUT_STATE *state) { /* same body */ }
  bool parseDualSenseReport(const std::vector<BYTE> &report, XINPUT_STATE *state) { /* same body */ }
} // namespace NexPadInternal
```

Function bodies are identical — only the enclosing namespace changes.

- [ ] **Step 3: Update call sites in `GetState()` to use qualified names**

In `CXBOXController.cpp`, find calls to `parseDualSenseReport(...)` (inside `GetState()`). Prefix them:

```cpp
// Before:
if (parseDualSenseReport(report, &_controllerState)) { ... }

// After:
if (NexPadInternal::parseDualSenseReport(report, &_controllerState)) { ... }
```

- [ ] **Step 4: Build Release|x64 to verify no regressions**

```powershell
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64 /t:NexPad /m
```

Expected: `Build succeeded.` with no errors or warnings that weren't present before.

- [ ] **Step 5: Commit**

```bash
git add Windows/NexPad/CXBOXController.h Windows/NexPad/CXBOXController.cpp
git commit -m "refactor: promote DualSense HID parsers to NexPadInternal namespace"
```

---

## Task 3: Add `NexPad` button state accessors

**Files:**
- Modify: `Windows/NexPad/NexPad.h`
- Modify: `Windows/NexPad/NexPad.cpp`

**Context:** `NexPad` already has `setXboxClickState(DWORD state)` public (line ~187). The private maps `_xboxClickIsDown`, `_xboxClickIsUp`, `_xboxClickIsDownLong` need read-only accessors so tests can inspect state without triggering Win32 side effects.

- [ ] **Step 1: Add accessor declarations to `NexPad.h`**

In `Windows/NexPad/NexPad.h`, in the `public:` section immediately after the `setXboxClickState` declaration, add:

```cpp
bool isButtonDown(DWORD key) const;
bool isButtonUp(DWORD key) const;
bool isButtonDownLong(DWORD key) const;
```

- [ ] **Step 2: Implement the three accessors in `NexPad.cpp`**

Add at the bottom of `Windows/NexPad/NexPad.cpp` (before the final closing brace if any, otherwise just append):

```cpp
bool NexPad::isButtonDown(DWORD key) const {
    auto it = _xboxClickIsDown.find(key);
    return it != _xboxClickIsDown.end() && it->second;
}

bool NexPad::isButtonUp(DWORD key) const {
    auto it = _xboxClickIsUp.find(key);
    return it != _xboxClickIsUp.end() && it->second;
}

bool NexPad::isButtonDownLong(DWORD key) const {
    auto it = _xboxClickIsDownLong.find(key);
    return it != _xboxClickIsDownLong.end() && it->second;
}
```

- [ ] **Step 3: Build Release|x64 to verify**

```powershell
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64 /t:NexPad /m
```

Expected: `Build succeeded.`

- [ ] **Step 4: Commit**

```bash
git add Windows/NexPad/NexPad.h Windows/NexPad/NexPad.cpp
git commit -m "feat: add NexPad button state accessors for test inspection"
```

---

## Task 4: Create `NexPadTests` VS project + skeleton test files

**Files:**
- Create: `Windows/NexPadTests/NexPadTests.vcxproj`
- Create: `Windows/NexPadTests/test_configfile.cpp` (skeleton)
- Create: `Windows/NexPadTests/test_controller_parsing.cpp` (skeleton)
- Create: `Windows/NexPadTests/test_button_state.cpp` (skeleton)
- Modify: `Windows/NexPad.sln`

- [ ] **Step 1: Create `Windows/NexPadTests/` directory**

```bash
mkdir -p Windows/NexPadTests
```

- [ ] **Step 2: Create `Windows/NexPadTests/NexPadTests.vcxproj`**

Create the file with this exact content:

```xml
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="15.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|Win32">
      <Configuration>Debug</Configuration>
      <Platform>Win32</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|Win32">
      <Configuration>Release</Configuration>
      <Platform>Win32</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Debug|x64">
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x64">
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <ProjectGuid>{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}</ProjectGuid>
    <Keyword>Win32Proj</Keyword>
    <RootNamespace>NexPadTests</RootNamespace>
    <WindowsTargetPlatformVersion>10.0</WindowsTargetPlatformVersion>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <CharacterSet>Unicode</CharacterSet>
    <PlatformToolset>v143</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|Win32'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <CharacterSet>Unicode</CharacterSet>
    <PlatformToolset>v143</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>true</UseDebugLibraries>
    <CharacterSet>Unicode</CharacterSet>
    <PlatformToolset>v143</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'" Label="Configuration">
    <ConfigurationType>Application</ConfigurationType>
    <UseDebugLibraries>false</UseDebugLibraries>
    <CharacterSet>Unicode</CharacterSet>
    <PlatformToolset>v143</PlatformToolset>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings">
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Release|Win32'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <ImportGroup Label="PropertySheets" Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" Label="LocalAppDataPlatform" />
  </ImportGroup>
  <PropertyGroup Label="UserMacros" />
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
    <OutDir>$(SolutionDir)..\debug\x32\</OutDir>
    <IntDir>$(SolutionDir).build\$(MSBuildProjectName)\$(Platform)\$(Configuration)\</IntDir>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|Win32'">
    <OutDir>$(SolutionDir)..\release\x32\</OutDir>
    <IntDir>$(SolutionDir).build\$(MSBuildProjectName)\$(Platform)\$(Configuration)\</IntDir>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <OutDir>$(SolutionDir)..\debug\x64\</OutDir>
    <IntDir>$(SolutionDir).build\$(MSBuildProjectName)\$(Platform)\$(Configuration)\</IntDir>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <OutDir>$(SolutionDir)..\release\x64\</OutDir>
    <IntDir>$(SolutionDir).build\$(MSBuildProjectName)\$(Platform)\$(Configuration)\</IntDir>
  </PropertyGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
    <ClCompile>
      <PrecompiledHeader></PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <Optimization>Disabled</Optimization>
      <PreprocessorDefinitions>WIN32;_DEBUG;GTEST_HAS_SEH=0;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>$(SolutionDir)NexPad\;$(SolutionDir)..\googletest\googletest\include\;$(SolutionDir)..\googletest\googletest\;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
      <AdditionalDependencies>xinput.lib;hid.lib;user32.lib;ole32.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|Win32'">
    <ClCompile>
      <PrecompiledHeader></PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <Optimization>MaxSpeed</Optimization>
      <PreprocessorDefinitions>WIN32;NDEBUG;GTEST_HAS_SEH=0;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>$(SolutionDir)NexPad\;$(SolutionDir)..\googletest\googletest\include\;$(SolutionDir)..\googletest\googletest\;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
      <AdditionalDependencies>xinput.lib;hid.lib;user32.lib;ole32.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <ClCompile>
      <PrecompiledHeader></PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <Optimization>Disabled</Optimization>
      <PreprocessorDefinitions>_DEBUG;GTEST_HAS_SEH=0;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>$(SolutionDir)NexPad\;$(SolutionDir)..\googletest\googletest\include\;$(SolutionDir)..\googletest\googletest\;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
      <AdditionalDependencies>xinput.lib;hid.lib;user32.lib;ole32.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <ClCompile>
      <PrecompiledHeader></PrecompiledHeader>
      <WarningLevel>Level3</WarningLevel>
      <Optimization>MaxSpeed</Optimization>
      <PreprocessorDefinitions>NDEBUG;GTEST_HAS_SEH=0;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalIncludeDirectories>$(SolutionDir)NexPad\;$(SolutionDir)..\googletest\googletest\include\;$(SolutionDir)..\googletest\googletest\;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
    </ClCompile>
    <Link>
      <SubSystem>Console</SubSystem>
      <GenerateDebugInformation>true</GenerateDebugInformation>
      <AdditionalDependencies>xinput.lib;hid.lib;user32.lib;ole32.lib;%(AdditionalDependencies)</AdditionalDependencies>
    </Link>
  </ItemDefinitionGroup>
  <ItemGroup>
    <ClCompile Include="..\..\googletest\googletest\src\gtest-all.cc" />
    <ClCompile Include="..\..\googletest\googletest\src\gtest_main.cc" />
    <ClCompile Include="..\NexPad\CXBOXController.cpp" />
    <ClCompile Include="..\NexPad\ConfigFile.cpp" />
    <ClCompile Include="..\NexPad\NexPad.cpp" />
    <ClCompile Include="test_configfile.cpp" />
    <ClCompile Include="test_controller_parsing.cpp" />
    <ClCompile Include="test_button_state.cpp" />
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <ImportGroup Label="ExtensionTargets">
  </ImportGroup>
</Project>
```

- [ ] **Step 3: Create skeleton test files**

Create `Windows/NexPadTests/test_configfile.cpp`:

```cpp
#include "gtest/gtest.h"
// Tests implemented in Task 5
```

Create `Windows/NexPadTests/test_controller_parsing.cpp`:

```cpp
#include "gtest/gtest.h"
// Tests implemented in Task 6
```

Create `Windows/NexPadTests/test_button_state.cpp`:

```cpp
#include "gtest/gtest.h"
// Tests implemented in Task 7
```

- [ ] **Step 4: Add `NexPadTests` to `Windows/NexPad.sln`**

Edit `Windows/NexPad.sln`. After the existing `EndProject` line (after the NexPad project entry), add:

```
Project("{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}") = "NexPadTests", "NexPadTests\NexPadTests.vcxproj", "{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}"
EndProject
```

Then in `GlobalSection(ProjectConfigurationPlatforms)`, add entries for the new GUID:

```
		{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}.Debug|Any CPU.ActiveCfg = Debug|Win32
		{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}.Debug|Win32.ActiveCfg = Debug|Win32
		{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}.Debug|Win32.Build.0 = Debug|Win32
		{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}.Debug|x64.ActiveCfg = Debug|x64
		{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}.Debug|x64.Build.0 = Debug|x64
		{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}.Release|Any CPU.ActiveCfg = Release|Win32
		{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}.Release|Win32.ActiveCfg = Release|Win32
		{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}.Release|Win32.Build.0 = Release|Win32
		{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}.Release|x64.ActiveCfg = Release|x64
		{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}.Release|x64.Build.0 = Release|x64
```

- [ ] **Step 5: Build the NexPadTests project (skeleton)**

```powershell
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64 /t:NexPadTests /m
```

Expected: `Build succeeded.` The output binary `release/x64/NexPadTests.exe` should exist.

- [ ] **Step 6: Run the skeleton binary to verify gtest wires up**

```powershell
.\release\x64\NexPadTests.exe
```

Expected output includes `[==========] 0 tests from 0 test suites ran.` (no tests yet).

- [ ] **Step 7: Commit**

```bash
git add Windows/NexPadTests/ Windows/NexPad.sln
git commit -m "feat: add NexPadTests VS project skeleton"
```

---

## Task 5: Implement `test_configfile.cpp`

**Files:**
- Modify: `Windows/NexPadTests/test_configfile.cpp`

**Context:** `ConfigFile` constructor takes a filename path (`const std::string& fName`). Each test writes a temp file using Win32 `GetTempPath`/`GetTempFileName`, constructs `ConfigFile` with that path, asserts behavior, then deletes the file. `getValueOfKey<T>()` takes a key string and a default value.

- [ ] **Step 1: Replace skeleton with full test file**

```cpp
#include "gtest/gtest.h"
#include "ConfigFile.h"
#include <windows.h>
#include <fstream>
#include <string>

// Helper: write content to a temp file, return its path.
// Caller must DeleteFileA() when done.
static std::string writeTempIni(const std::string& content) {
    char tempDir[MAX_PATH];
    char tempFile[MAX_PATH];
    GetTempPathA(MAX_PATH, tempDir);
    GetTempFileNameA(tempDir, "npt", 0, tempFile);
    std::ofstream f(tempFile);
    f << content;
    return std::string(tempFile);
}

TEST(ConfigFile, ParsesKeyValue) {
    std::string path = writeTempIni("key = value\n");
    ConfigFile cfg(path);
    EXPECT_EQ(cfg.getValueOfKey<std::string>("key", ""), "value");
    DeleteFileA(path.c_str());
}

TEST(ConfigFile, StripsHashComments) {
    std::string path = writeTempIni("# this is a comment\nkey = hello\n");
    ConfigFile cfg(path);
    EXPECT_EQ(cfg.getValueOfKey<std::string>("key", ""), "hello");
    EXPECT_FALSE(cfg.keyExists("# this is a comment"));
    DeleteFileA(path.c_str());
}

TEST(ConfigFile, MissingKeyReturnsDefault) {
    std::string path = writeTempIni("key = value\n");
    ConfigFile cfg(path);
    EXPECT_EQ(cfg.getValueOfKey<std::string>("missing", "default"), "default");
    DeleteFileA(path.c_str());
}

TEST(ConfigFile, ParsesInt) {
    std::string path = writeTempIni("count = 42\n");
    ConfigFile cfg(path);
    EXPECT_EQ(cfg.getValueOfKey<int>("count", 0), 42);
    DeleteFileA(path.c_str());
}

TEST(ConfigFile, ParsesFloat) {
    std::string path = writeTempIni("speed = 0.5\n");
    ConfigFile cfg(path);
    EXPECT_FLOAT_EQ(cfg.getValueOfKey<float>("speed", 0.0f), 0.5f);
    DeleteFileA(path.c_str());
}

TEST(ConfigFile, HexValueParsedAsInt) {
    std::string path = writeTempIni("button = 0x0020\n");
    ConfigFile cfg(path);
    // getValueOfKey<DWORD> uses Convert::string_to_T which calls stoul with base 16 for hex strings
    EXPECT_EQ(cfg.getValueOfKey<DWORD>("button", 0), 0x0020u);
    DeleteFileA(path.c_str());
}
```

- [ ] **Step 2: Build and run**

```powershell
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64 /t:NexPadTests /m
.\release\x64\NexPadTests.exe --gtest_filter=ConfigFile.*
```

Expected: all `ConfigFile.*` tests pass. If `HexValueParsedAsInt` fails, check `Convert.h` to see if `string_to_T<DWORD>` uses `stoul`/`stoi` and adjust the test accordingly — the hex parsing behavior depends on the `Convert` implementation.

- [ ] **Step 3: Commit**

```bash
git add Windows/NexPadTests/test_configfile.cpp
git commit -m "test: add ConfigFile unit tests"
```

---

## Task 6: Implement `test_controller_parsing.cpp`

**Files:**
- Modify: `Windows/NexPadTests/test_controller_parsing.cpp`

**Context:** The three promoted parsers live in `NexPadInternal` namespace, declared in `CXBOXController.h`. XINPUT button constants: `XINPUT_GAMEPAD_A=0x1000`, `XINPUT_GAMEPAD_B=0x2000`, `XINPUT_GAMEPAD_X=0x4000`, `XINPUT_GAMEPAD_Y=0x8000`, `XINPUT_GAMEPAD_LEFT_SHOULDER=0x0100`, `XINPUT_GAMEPAD_RIGHT_SHOULDER=0x0200`.

`toThumbAxis(BYTE v) = (int(v)-128)*256`, so `0x80` → 0.

The parsers call `updateDualSenseBatteryStatus` and `updateDualSenseTouchpadState` (anonymous-namespace helpers in `CXBOXController.cpp`). These touch global state but won't crash — safe to call in tests.

- [ ] **Step 1: Replace skeleton with full test file**

```cpp
#include "gtest/gtest.h"
#include "CXBOXController.h"
#include <vector>
#include <cstdint>

// --- parseDualSenseUsbReport ---
// Layout (baseOffset=1): [0]=0x01, [1]=LX, [2]=LY, [3]=RX, [4]=RY,
//                        [5]=L2, [6]=R2, [7]=pad, [8]=buttons1, [9]=buttons2
// buttons1 bit5=Cross(A=0x1000). LX=0x80 -> sThumbLX=0. L2=0x40 -> bLeftTrigger=64.

TEST(DualSenseUsb, CrossButtonAndAxes) {
    std::vector<BYTE> report = {
        0x01,        // report ID
        0x80,        // LX=0x80 -> sThumbLX=(128-128)*256=0
        0x80,        // LY
        0x80,        // RX
        0x80,        // RY
        0x40,        // L2=64
        0x00,        // R2
        0x00,        // pad
        0x20,        // buttons1: bit5=Cross -> XINPUT_GAMEPAD_A (0x1000)
        0x00,        // buttons2
        0x00, 0x00   // pad to reach 12 bytes
    };
    XINPUT_STATE state = {};
    EXPECT_TRUE(NexPadInternal::parseDualSenseUsbReport(report, &state));
    EXPECT_EQ(state.Gamepad.wButtons, (WORD)XINPUT_GAMEPAD_A);
    EXPECT_EQ(state.Gamepad.sThumbLX, (SHORT)0);
    EXPECT_EQ(state.Gamepad.bLeftTrigger, (BYTE)64);
}

TEST(DualSenseUsb, ShortBufferReturnsFalse) {
    std::vector<BYTE> report = {0x01, 0x80, 0x80}; // only 3 bytes, min is 12
    XINPUT_STATE state = {};
    EXPECT_FALSE(NexPadInternal::parseDualSenseUsbReport(report, &state));
}

TEST(DualSenseUsb, WrongReportIdReturnsFalse) {
    std::vector<BYTE> report(12, 0x00); // report[0] != 0x01
    XINPUT_STATE state = {};
    EXPECT_FALSE(NexPadInternal::parseDualSenseUsbReport(report, &state));
}

// --- parseDualSenseBluetoothSimpleReport ---
// Layout: [0]=0x01, [1]=LX, [2]=LY, [3]=RX, [4]=RY,
//         [5]=buttons1, [6]=buttons2, [7]=pad, [8]=L2, [9]=R2
// buttons1 bit6=Circle(B=0x2000), buttons2 bit1=R1(0x0200). R2=0x80 -> 128.

TEST(DualSenseBtSimple, CircleAndR1Buttons) {
    std::vector<BYTE> report = {
        0x01,  // report ID
        0x80,  // LX
        0x80,  // LY
        0x80,  // RX
        0x80,  // RY
        0x40,  // buttons1: bit6=Circle -> XINPUT_GAMEPAD_B (0x2000)
        0x02,  // buttons2: bit1=R1 -> XINPUT_GAMEPAD_RIGHT_SHOULDER (0x0200)
        0x00,  // pad
        0x00,  // L2
        0x80   // R2=128
    };
    XINPUT_STATE state = {};
    EXPECT_TRUE(NexPadInternal::parseDualSenseBluetoothSimpleReport(report, &state));
    EXPECT_EQ(state.Gamepad.wButtons, (WORD)(XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_RIGHT_SHOULDER));
    EXPECT_EQ(state.Gamepad.bRightTrigger, (BYTE)128);
}

TEST(DualSenseBtSimple, ShortBufferReturnsFalse) {
    std::vector<BYTE> report = {0x01, 0x80}; // only 2 bytes, min is 10
    XINPUT_STATE state = {};
    EXPECT_FALSE(NexPadInternal::parseDualSenseBluetoothSimpleReport(report, &state));
}

// --- parseDualSenseBluetoothEnhancedReport ---
// Layout: [0]=0x31, [1]=pad, baseOffset=2 mirrors USB layout.
// buttons1 at index [8] (baseOffset+6), buttons2 at index [9] (baseOffset+7).
// Min size: 55 bytes (baseOffset+52+1).
// buttons1=0x80 bit7=Triangle -> XINPUT_GAMEPAD_Y (0x8000)
// buttons2=0x01 bit0=L1 -> XINPUT_GAMEPAD_LEFT_SHOULDER (0x0100)

TEST(DualSenseBtEnhanced, TriangleAndL1Buttons) {
    std::vector<BYTE> report(55, 0x00);
    report[0] = 0x31;  // BT enhanced report ID
    report[8] = 0x80;  // buttons1 (baseOffset+6=2+6=8): bit7=Triangle -> 0x8000
    report[9] = 0x01;  // buttons2 (baseOffset+7=2+7=9): bit0=L1 -> 0x0100
    XINPUT_STATE state = {};
    EXPECT_TRUE(NexPadInternal::parseDualSenseBluetoothEnhancedReport(report, &state));
    EXPECT_EQ(state.Gamepad.wButtons, (WORD)(XINPUT_GAMEPAD_Y | XINPUT_GAMEPAD_LEFT_SHOULDER));
}

TEST(DualSenseBtEnhanced, ShortBufferReturnsFalse) {
    std::vector<BYTE> report(10, 0x31); // too short (min 55)
    XINPUT_STATE state = {};
    EXPECT_FALSE(NexPadInternal::parseDualSenseBluetoothEnhancedReport(report, &state));
}

TEST(DualSenseBtEnhanced, WrongReportIdReturnsFalse) {
    std::vector<BYTE> report(55, 0x00); // report[0] != 0x31
    XINPUT_STATE state = {};
    EXPECT_FALSE(NexPadInternal::parseDualSenseBluetoothEnhancedReport(report, &state));
}
```

- [ ] **Step 2: Build and run**

```powershell
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64 /t:NexPadTests /m
.\release\x64\NexPadTests.exe --gtest_filter=DualSense*
```

Expected: all `DualSense*` tests pass.

- [ ] **Step 3: Commit**

```bash
git add Windows/NexPadTests/test_controller_parsing.cpp
git commit -m "test: add DualSense HID parser unit tests"
```

---

## Task 7: Implement `test_button_state.cpp`

**Files:**
- Modify: `Windows/NexPadTests/test_button_state.cpp`

**Context:** `NexPad::setXboxClickState(DWORD state)` updates the click maps each "tick". Calling it with a bitmask simulates the controller reporting that button as held. Calling it with `0` simulates all buttons released.

- Rising edge (`isButtonDown`): true only on the first call with the button held.
- Falling edge (`isButtonUp`): true only on the first call after the button was released.
- Long hold (`isButtonDownLong`): true after the button has been held >200 ms (tracked via `GetTickCount` internally). Tests verify this by sleeping 201ms between ticks.

`NexPad` constructor: `NexPad(CXBOXController* controller)`. Pass a real `CXBOXController(1)` instance — it won't connect to hardware, just stores the player number.

- [ ] **Step 1: Replace skeleton with full test file**

```cpp
#include "gtest/gtest.h"
#include "NexPad.h"
#include "CXBOXController.h"
#include <windows.h>  // for Sleep

// Fixture: creates a disconnected CXBOXController + NexPad.
// setXboxClickState() does NOT call GetState() or SendInput — safe in unit tests.
class ButtonStateTest : public ::testing::Test {
protected:
    CXBOXController controller{1};
    NexPad nexPad{&controller};
};

TEST_F(ButtonStateTest, RisingEdge_TrueOnFirstPress) {
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);
    EXPECT_TRUE(nexPad.isButtonDown(XINPUT_GAMEPAD_A));
    EXPECT_FALSE(nexPad.isButtonUp(XINPUT_GAMEPAD_A));
}

TEST_F(ButtonStateTest, RisingEdge_FalseOnSecondTick) {
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // tick 1: press
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // tick 2: still held
    EXPECT_FALSE(nexPad.isButtonDown(XINPUT_GAMEPAD_A));  // no longer rising
    EXPECT_FALSE(nexPad.isButtonUp(XINPUT_GAMEPAD_A));
}

TEST_F(ButtonStateTest, FallingEdge_TrueOnRelease) {
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // press
    nexPad.setXboxClickState(0);                  // release
    EXPECT_TRUE(nexPad.isButtonUp(XINPUT_GAMEPAD_A));
    EXPECT_FALSE(nexPad.isButtonDown(XINPUT_GAMEPAD_A));
}

TEST_F(ButtonStateTest, FallingEdge_FalseOnSecondTickAfterRelease) {
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // press
    nexPad.setXboxClickState(0);                  // release
    nexPad.setXboxClickState(0);                  // still released
    EXPECT_FALSE(nexPad.isButtonUp(XINPUT_GAMEPAD_A));
}

TEST_F(ButtonStateTest, LongHold_TrueAfter200ms) {
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // press (tick 1)
    Sleep(201);                                   // wait > 200 ms
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // held (tick 2 — now beyond threshold)
    EXPECT_TRUE(nexPad.isButtonDownLong(XINPUT_GAMEPAD_A));
}

TEST_F(ButtonStateTest, LongHold_FalseBeforeThreshold) {
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // press
    nexPad.setXboxClickState(XINPUT_GAMEPAD_A);  // held immediately
    EXPECT_FALSE(nexPad.isButtonDownLong(XINPUT_GAMEPAD_A));
}

TEST_F(ButtonStateTest, Combo_BothBitsRequired) {
    const DWORD combo = XINPUT_GAMEPAD_START | XINPUT_GAMEPAD_BACK; // 0x0030
    nexPad.setXboxClickState(combo);
    EXPECT_TRUE(nexPad.isButtonDown(combo));
    EXPECT_FALSE(nexPad.isButtonDown(XINPUT_GAMEPAD_START));  // individual keys not tracked separately
    EXPECT_FALSE(nexPad.isButtonDown(XINPUT_GAMEPAD_BACK));
}
```

Note on the combo test: `setXboxClickState` tracks state keyed by the exact bitmask passed. The combo `0x0030` is only tracked if the caller passes `0x0030` — individual button bits are not split out. This mirrors how `NexPad::loop()` calls `setXboxClickState` with pre-computed combo keys.

- [ ] **Step 2: Build and run**

```powershell
msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64 /t:NexPadTests /m
.\release\x64\NexPadTests.exe --gtest_filter=ButtonState*
```

Expected: all `ButtonState*` tests pass. The `LongHold_TrueAfter200ms` test will take ~201ms to run — that is expected.

- [ ] **Step 3: Run full suite**

```powershell
.\release\x64\NexPadTests.exe
```

Expected: all tests pass (`ConfigFile.*`, `DualSense*`, `ButtonState*`).

- [ ] **Step 4: Commit**

```bash
git add Windows/NexPadTests/test_button_state.cpp
git commit -m "test: add NexPad button state edge detection unit tests"
```

---

## Task 8: Wire CI

**Files:**
- Modify: `.github/workflows/build.yml`

- [ ] **Step 1: Add `submodules: recursive` to the checkout step**

In `.github/workflows/build.yml`, change the Checkout step from:

```yaml
      - name: Checkout
        uses: actions/checkout@v4
```

To:

```yaml
      - name: Checkout
        uses: actions/checkout@v4
        with:
          submodules: recursive
```

- [ ] **Step 2: Add a test build + run step after "Build and package release artifacts"**

Add this step after the `Build and package release artifacts` step and before `Verify packaged artifact checksums`:

```yaml
      - name: Build and run NexPadTests
        shell: pwsh
        run: |
          msbuild Windows/NexPad.sln /p:Configuration=Release /p:Platform=x64 /t:NexPadTests /m
          .\release\x64\NexPadTests.exe --gtest_output=xml:test-results.xml

      - name: Upload test results
        if: always()
        uses: actions/upload-artifact@v4
        with:
          name: test-results-${{ steps.version_label.outputs.value }}
          path: test-results.xml
```

The `if: always()` on the upload ensures test results are uploaded even when tests fail, so failures can be diagnosed.

- [ ] **Step 3: Commit**

```bash
git add .github/workflows/build.yml
git commit -m "ci: add NexPadTests build and run step; add submodules: recursive"
```

- [ ] **Step 4: Push and verify CI passes**

```bash
git push origin main
```

Monitor the Actions run. Expected: all steps pass, including `Build and run NexPadTests`. Test results artifact is uploaded.

---

## Done When

- `NexPadTests.exe` builds from `NexPad.sln` targeting Release|x64
- All tests pass locally: `ConfigFile.*`, `DualSense*`, `ButtonState*`
- CI workflow passes on push to `main` with the new test step
- `release/x64/NexPadTests.exe` is produced alongside `NexPad.exe`
