@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "CLEAN="
if /i "%~1"=="clean" set "CLEAN=1"
if /i "%~1"=="--clean" set "CLEAN=1"
if /i "%~1"=="/clean" set "CLEAN=1"

set "SCRIPT_DIR=%~dp0"
set "REPO_ROOT=%SCRIPT_DIR%.."
set "SOLUTION=%REPO_ROOT%\Windows\NexPad.sln"
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "MSBUILD="

if not exist "%SOLUTION%" (
  echo Solution not found: "%SOLUTION%"
  exit /b 1
)

if exist "%VSWHERE%" (
  for /f "usebackq delims=" %%I in (`"%VSWHERE%" -latest -products * -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do (
    if not defined MSBUILD set "MSBUILD=%%I"
  )
)

if not defined MSBUILD (
  for /f "usebackq delims=" %%I in (`where msbuild 2^>nul`) do (
    if not defined MSBUILD set "MSBUILD=%%I"
  )
)

if not defined MSBUILD (
  echo MSBuild.exe was not found. Install Visual Studio Build Tools 2022 or add MSBuild to PATH.
  exit /b 1
)

echo Using MSBuild: "%MSBUILD%"
echo Solution: "%SOLUTION%"
echo.

for %%C in (Debug Release) do (
  for %%P in (Win32 x64) do (
    if defined CLEAN (
      echo ============================================================
      echo Cleaning %%C^|%%P
      echo ============================================================
      call "%MSBUILD%" "%SOLUTION%" /t:Clean /p:Configuration=%%C /p:Platform=%%P
      if errorlevel 1 (
        echo.
        echo Clean failed for %%C^|%%P
        exit /b 1
      )
      echo.
    )

    echo ============================================================
    echo Building %%C^|%%P
    echo ============================================================
    call "%MSBUILD%" "%SOLUTION%" /p:Configuration=%%C /p:Platform=%%P
    if errorlevel 1 (
      echo.
      echo Build failed for %%C^|%%P
      exit /b 1
    )
    echo.
  )
)

if defined CLEAN (
  echo Completed clean and build for all available solution targets.
  exit /b 0
)

echo All available solution builds completed successfully.
exit /b 0