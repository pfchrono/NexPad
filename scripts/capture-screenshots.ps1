[CmdletBinding()]
param(
  [Parameter()]
  [string]$ExePath = '',

  [Parameter()]
  [string]$OutputDirectory = ''
)

$ErrorActionPreference = 'Stop'

function Resolve-DefaultExePath {
  param(
    [Parameter(Mandatory = $true)]
    [string]$RepoRoot
  )

  $candidatePaths = @(
    (Join-Path $RepoRoot 'release\x64\NexPad.exe'),
    (Join-Path $RepoRoot 'release\x32\NexPad.exe'),
    (Join-Path $RepoRoot 'debug\x64\NexPad.exe'),
    (Join-Path $RepoRoot 'debug\x32\NexPad.exe')
  )

  foreach ($candidatePath in $candidatePaths) {
    if (Test-Path $candidatePath) {
      return $candidatePath
    }
  }

  throw 'No built NexPad executable was found. Build the solution first or pass -ExePath explicitly.'
}

Add-Type -AssemblyName System.Drawing
if (-not ('NexPadCaptureNative' -as [type])) {
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public static class NexPadCaptureNative
{
    public delegate bool EnumChildProc(IntPtr hwnd, IntPtr lParam);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct NMHDR
    {
        public IntPtr hwndFrom;
        public UIntPtr idFrom;
        public int code;
    }

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool EnumChildWindows(IntPtr hWndParent, EnumChildProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT rect);

    [DllImport("user32.dll")]
    public static extern bool MoveWindow(IntPtr hWnd, int x, int y, int nWidth, int nHeight, bool repaint);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);

    [DllImport("user32.dll")]
    public static extern bool InvalidateRect(IntPtr hWnd, IntPtr rect, bool erase);

    [DllImport("user32.dll")]
    public static extern bool UpdateWindow(IntPtr hWnd);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessage(IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);
}
"@
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if ([string]::IsNullOrWhiteSpace($ExePath)) {
  $ExePath = Resolve-DefaultExePath -RepoRoot $repoRoot
}

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
  $OutputDirectory = Join-Path $repoRoot 'docs\assets\screenshots'
}

function Get-MainWindowHandle {
  param(
    [Parameter(Mandatory = $true)]
    [System.Diagnostics.Process]$Process
  )

  $deadline = (Get-Date).AddSeconds(15)
  while ($Process.MainWindowHandle -eq 0 -and (Get-Date) -lt $deadline) {
    Start-Sleep -Milliseconds 250
    $Process.Refresh()
  }

  if ($Process.MainWindowHandle -eq 0) {
    throw 'NexPad window did not appear.'
  }

  return $Process.MainWindowHandle
}

function Get-TabHandle {
  param(
    [Parameter(Mandatory = $true)]
    [IntPtr]$MainWindow
  )

  $script:resolvedTab = [IntPtr]::Zero
  $callback = [NexPadCaptureNative+EnumChildProc]{
    param($hwnd, $lParam)
    $builder = New-Object System.Text.StringBuilder 64
    [void][NexPadCaptureNative]::GetClassName($hwnd, $builder, $builder.Capacity)
    if ($builder.ToString() -eq 'SysTabControl32') {
      $script:resolvedTab = $hwnd
      return $false
    }
    return $true
  }

  [NexPadCaptureNative]::EnumChildWindows($MainWindow, $callback, [IntPtr]::Zero) | Out-Null
  if ($script:resolvedTab -eq [IntPtr]::Zero) {
    throw 'Could not find the NexPad tab control.'
  }

  return $script:resolvedTab
}

function Set-NexPadTab {
  param(
    [Parameter(Mandatory = $true)]
    [IntPtr]$MainWindow,

    [Parameter(Mandatory = $true)]
    [IntPtr]$TabHandle,

    [Parameter(Mandatory = $true)]
    [int]$Index
  )

  $WM_MOUSEMOVE = 0x0200
  $WM_LBUTTONDOWN = 0x0201
  $WM_LBUTTONUP = 0x0202
  $MK_LBUTTON = 0x0001
  $centerX = 48 + ($Index * 92)
  $centerY = 14
  $lParam = [IntPtr](($centerY -shl 16) -bor ($centerX -band 0xFFFF))

  [void][NexPadCaptureNative]::SendMessage($TabHandle, $WM_MOUSEMOVE, [IntPtr]::Zero, $lParam)
  [void][NexPadCaptureNative]::SendMessage($TabHandle, $WM_LBUTTONDOWN, [IntPtr]$MK_LBUTTON, $lParam)
  [void][NexPadCaptureNative]::SendMessage($TabHandle, $WM_LBUTTONUP, [IntPtr]::Zero, $lParam)
  [NexPadCaptureNative]::InvalidateRect($MainWindow, [IntPtr]::Zero, $true) | Out-Null
  [NexPadCaptureNative]::UpdateWindow($MainWindow) | Out-Null
  [NexPadCaptureNative]::SetForegroundWindow($MainWindow) | Out-Null
  Start-Sleep -Milliseconds 700
}

function Save-WindowCapture {
  param(
    [Parameter(Mandatory = $true)]
    [IntPtr]$MainWindow,

    [Parameter(Mandatory = $true)]
    [string]$Path
  )

  $rect = New-Object NexPadCaptureNative+RECT
  [void][NexPadCaptureNative]::GetWindowRect($MainWindow, [ref]$rect)
  $width = $rect.Right - $rect.Left
  $height = $rect.Bottom - $rect.Top

  $bitmap = New-Object System.Drawing.Bitmap $width, $height
  $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
  $graphics.CopyFromScreen($rect.Left, $rect.Top, 0, 0, (New-Object System.Drawing.Size($width, $height)))
  $bitmap.Save($Path, [System.Drawing.Imaging.ImageFormat]::Png)
  $graphics.Dispose()
  $bitmap.Dispose()
}

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null

$launchedProcess = $null
if (!(Test-Path $ExePath)) {
  throw "Executable not found: $ExePath"
}

$launchedProcess = Start-Process -FilePath $ExePath -PassThru
$process = $launchedProcess

try {
  $mainWindow = Get-MainWindowHandle -Process $process
  [NexPadCaptureNative]::ShowWindow($mainWindow, 5) | Out-Null
  [NexPadCaptureNative]::MoveWindow($mainWindow, 80, 80, 760, 600, $true) | Out-Null
  [NexPadCaptureNative]::SetForegroundWindow($mainWindow) | Out-Null
  Start-Sleep -Milliseconds 700

  $tabHandle = Get-TabHandle -MainWindow $mainWindow

  Set-NexPadTab -MainWindow $mainWindow -TabHandle $tabHandle -Index 0
  Save-WindowCapture -MainWindow $mainWindow -Path (Join-Path $OutputDirectory 'status-tab.png')
  Write-Host "Saved $(Join-Path $OutputDirectory 'status-tab.png')"

  Set-NexPadTab -MainWindow $mainWindow -TabHandle $tabHandle -Index 1
  Save-WindowCapture -MainWindow $mainWindow -Path (Join-Path $OutputDirectory 'settings-tab.png')
  Write-Host "Saved $(Join-Path $OutputDirectory 'settings-tab.png')"

  Set-NexPadTab -MainWindow $mainWindow -TabHandle $tabHandle -Index 2
  Save-WindowCapture -MainWindow $mainWindow -Path (Join-Path $OutputDirectory 'mappings-tab.png')
  Write-Host "Saved $(Join-Path $OutputDirectory 'mappings-tab.png')"
}
finally {
  if ($launchedProcess -and !$launchedProcess.HasExited) {
    $launchedProcess.CloseMainWindow() | Out-Null
    Start-Sleep -Milliseconds 500
    if (!$launchedProcess.HasExited) {
      Stop-Process -Id $launchedProcess.Id -Force
    }
  }
}